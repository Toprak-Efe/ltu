#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <optional>

#include <boost/process/v1.hpp>
#include <boost/process/v1/extend.hpp>
#include <INIReader.h>
#include <desktop.hpp>
#include <terminal.hpp>

namespace ltu {
    std::vector<std::filesystem::path> parse_path_var() {
        const char* path_variable = getenv("PATH");
        if (!path_variable) return {};
        std::vector<std::string_view> parsed_strings;
        size_t idx = 0, prev_idx = 0;
        while (true) {
            if (path_variable[idx] == ':') {
                parsed_strings.emplace_back(path_variable + prev_idx, idx - prev_idx);
                prev_idx = idx + 1;
            }
            if (path_variable[idx] == '\0') {
                if (path_variable[prev_idx] != '\0')
                    parsed_strings.emplace_back(path_variable + prev_idx, idx - prev_idx);
                break;
            }
            idx++;
        }
        std::vector<std::filesystem::path> out;
        for (const auto &path_sv : parsed_strings) {
            out.emplace_back(std::filesystem::path(path_sv));
        }
        return out;
    } 
    
    std::optional<std::filesystem::path> get_absolute_path(const std::string &command) {
        namespace fs = std::filesystem;
        fs::path command_path(command);
        if (command_path.is_absolute() && fs::is_regular_file(command_path)) return command_path;
        auto path_executable_dirs = ltu::parse_path_var();
        for (fs::path path_executable_dir : path_executable_dirs) {
            fs::path executable_candidate = path_executable_dir / command_path; 
            if (fs::is_regular_file(executable_candidate)) return executable_candidate;
        }
        return {};
    }
} // namespace ltu

std::optional<ltu::desktop_entry_t> ltu::parse_desktop_file(const std::filesystem::path &desktop_file) {
    INIReader reader(desktop_file);
    std::string type = reader.Get("Desktop Entry", "Type", "");
    if (type != "Application") return {};
    desktop_entry_t out;
    out.name = reader.Get("Desktop Entry", "Name", "Unnamed");
    out.comment = reader.Get("Desktop Entry", "Comment", "No description.");
    out.terminal = reader.GetBoolean("Desktop Entry", "Terminal", false);

    std::string exec_str = reader.Get("Desktop Entry", "Exec", "");
    if (exec_str.size() == 0) return {};
    size_t p_idx = 0;
    while ((p_idx = exec_str.find('%')) != std::string::npos) {
        exec_str[p_idx] = ' ';
        if (p_idx != exec_str.length()-1) exec_str[p_idx+1] = ' ';
    }
    size_t strip_begin = 0, strip_end = exec_str.size() - 1;
    while (exec_str[strip_begin] == ' ') strip_begin++; 
    while (exec_str[strip_end] == ' ' && strip_end > 0) strip_end--; 
    exec_str = exec_str.substr(strip_begin, strip_end - strip_begin + 1);
    out.exec = exec_str;

    out.path = desktop_file;
    return out;
}
    
int ltu::run_desktop_entry(const ltu::desktop_entry_t &desktop_entry, const ltu::terminal_profile_t &terminal_profile) {
    namespace bp = boost::process::v1;
    namespace fs = std::filesystem;
    
    std::string command;
    std::optional<fs::path> exec_path_opt = get_absolute_path(desktop_entry.exec);
    if (!exec_path_opt.has_value()) {
        std::cerr << "Couldn't find the executable " << desktop_entry.exec << ", exiting.\n";
        return 1;
    }
    if (desktop_entry.terminal) {
        std::optional<fs::path> terminal_path_opt = get_absolute_path(terminal_profile.binary);
        if (!terminal_path_opt.has_value()) {
            std::cerr << "Couldn't find the terminal program " << terminal_profile.binary << ", exiting.\n";
            return 1;
        }
        command = terminal_path_opt.value().string() + " " + terminal_profile.flag + " ";
    }
    command = command + exec_path_opt.value().string();
    std::cout << "Running command: " << command << "\n";

    bp::child c(
        command,
        bp::std_out > bp::null,
        bp::std_err > bp::null,
        bp::std_in < bp::null,
        bp::extend::on_setup([](auto&) {
            ::setsid();
        })
    );
    c.detach();

    return 0;
}

