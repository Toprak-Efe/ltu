#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process/v1/exe.hpp>
#include <boost/regex.hpp>
#include <boost/regex/v5/regex_replace.hpp>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>
#include <optional>

#include <boost/process/v1.hpp>
#include <boost/process/v1/extend.hpp>
#include <INIReader.h>
#include <desktop.hpp>
#include <terminal.hpp>

namespace ltu {

    bool is_executable(const std::string &path) {
        return std::filesystem::is_regular_file(path) &&
            (::access(path.c_str(), X_OK) == 0);
    }

    std::vector<std::filesystem::path> parse_path_var() {
        const char* path_variable = getenv("PATH");
        if (!path_variable) return {};
        std::string_view path_sv(path_variable);

        std::vector<std::filesystem::path> parsed_paths;

        size_t start = 0, end = 0;
        while ((end = path_sv.find(':', start)) != std::string_view::npos) {
            if (end != start && end != start + 1) {
                parsed_paths.emplace_back(path_sv.substr(start, end - start));
            }
            start = end + 1;
        }
        if (start < path_sv.size()) {
            parsed_paths.emplace_back(path_sv.substr(start));
        }

        return parsed_paths;
    } 
    
    std::optional<std::filesystem::path> get_absolute_path(const std::string &command) {
        namespace fs = std::filesystem;
        fs::path command_path(command);

        if (command_path.is_absolute() && fs::is_regular_file(command_path)) return command_path;
        static const std::vector<fs::path> path_dirs = parse_path_var();

        for (const auto & dir : path_dirs) {
            fs::path candidate = dir / command_path; 
            if (is_executable(candidate)) return candidate;
        }
        return {};
    }

    std::vector<std::string> tokenize_command(const std::string &command) {
        std::vector<std::string> args;
        std::string curr_token;

        bool double_quote(false), single_quote(false), escaped(false);

        for (char c : command) {
            if (escaped) {
                curr_token += c;
                escaped = false;
                continue;
            }
            if (c == '\\') {
                escaped = true;
                continue;
            }
            if (double_quote) {
                if (c == '\"') {
                    double_quote = false;
                } else {
                    curr_token += c;
                }
                continue;
            }
            if (single_quote) {
                if (c == '\'') {
                    single_quote = false;
                } else {
                    curr_token += c;
                }
                continue;
            }

            switch (c) {
                case ' ':
                case '\t':
                    if (!curr_token.empty()) {
                        args.emplace_back(curr_token);
                        curr_token.clear(); 
                        break;
                    }
                case '\'':
                    single_quote = true;
                    break;
                case '\"':
                    double_quote = true;
                    break;
                case '\\':
                    escaped = true;
                    break;
                default:
                    curr_token += c;
                    break;
            }
        }
        if (!curr_token.empty()) {
            args.emplace_back(curr_token);
        }

        return args;
    }

} // namespace ltu

std::optional<ltu::desktop_entry_t> ltu::parse_desktop_file(const std::filesystem::path &desktop_file) {
    INIReader reader(desktop_file.string());
    if (reader.ParseError() < -1) {
        std::cerr << "Unable to open file " << desktop_file << " for parsing.\n";
        return {};
    };

    std::string type = reader.Get("Desktop Entry", "Type", "");
    if (type != "Application") return {};

    desktop_entry_t out;
    out.name = reader.Get("Desktop Entry", "Name", "Unnamed");
    out.comment = reader.Get("Desktop Entry", "Comment", "No description.");
    out.terminal = reader.GetBoolean("Desktop Entry", "Terminal", false);
    out.path = desktop_file;

    std::string exec_str = reader.Get("Desktop Entry", "Exec", "");
    if (exec_str.empty()) return {}; 

    boost::trim(exec_str);

    static const boost::regex formatting_pattern("%[uUFi]?");
    exec_str = boost::regex_replace(exec_str, formatting_pattern, "");
    boost::trim(exec_str);

    std::vector<std::string> args = tokenize_command(exec_str);
    if (args.empty()) return {};

    out.exec = args[0];
    if (args.size() > 1) {
        out.args.assign(args.begin() + 1, args.end()); 
    }

    return out;
}

int ltu::run_desktop_entry(const ltu::desktop_entry_t &desktop_entry, const ltu::terminal_profile_t &terminal_profile) {
    namespace bp = boost::process::v1;
    namespace fs = std::filesystem;

    std::string command;
    std::optional<fs::path> exec_path_opt = get_absolute_path(desktop_entry.exec);
    if (!exec_path_opt.has_value()) {
        std::clog << "Couldn't find the executable " << desktop_entry.exec << ", exiting.\n";
        return 1;
    }

    fs::path binary;
    std::vector<std::string> args;

    if (desktop_entry.terminal) {
        std::optional<fs::path> term_path = get_absolute_path(terminal_profile.binary);
        if (!term_path.has_value()) {
            std::clog << "Couldn't find the terminal program " << terminal_profile.binary << ", exiting.\n";
            return 1;
        }
        binary = term_path.value();
        std::stringstream ss(terminal_profile.flag);
        std::string segment;
        while (std::getline(ss, segment, ' ')) {
            if (!segment.empty()) args.push_back(segment);
        }
        args.push_back(exec_path_opt.value().string());
        args.insert(args.end(), desktop_entry.args.begin(), desktop_entry.args.end());
    } else {
        binary = exec_path_opt.value();
        args= desktop_entry.args;
    }

    try {
        bp::child c(
            binary.string(),
            bp::args(args), 
            bp::std_out > bp::null,
            bp::std_err > bp::null,
            bp::std_in < bp::null,
            bp::extend::on_setup([](auto&) {
                ::setsid();
                ::signal(SIGHUP, SIG_IGN); 
            })
        );
        c.detach();
    } catch (const std::exception& e) {
        std::clog << "Failed to launch process: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

