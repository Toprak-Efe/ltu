#include <cstdlib>
#include <map>
#include <array>
#include <ranges>
#include <filesystem>
#include <string_view>
#include <iostream>

#include <desktop.hpp>
#include <terminal.hpp>
#include <menu.hpp>

namespace ltu {
    consteval const auto desktop_file_paths () {
        using namespace std::literals::string_view_literals;
        using namespace std::filesystem;
        return std::to_array({
            "~/.local/share/applications"sv,
            "/usr/share/applications"sv,
            "/usr/local/share/applications/"sv,
        }) ;
    };

    void usage_error() {
        std::clog << "Bad usage. Example:\n";
        std::clog << "ltu <terminal>\n\n";
        std::clog << "<terminal>: The terminal emulator all CLI applications should be started on.\nOptions:\n";
        for (std::string &term : ltu::g_known_terminals
            | std::ranges::views::keys
            | std::ranges::to<std::vector>()) {
            std::clog << "  " << term << "\n"; 
        }
        std::exit(1);
    }

}; // namespace ltu

int main (int argc, char *argv[]) {
    /* Fetch Terminal Info */
    if (argc != 2) {
        ltu::usage_error();
    }
    auto terminal_profile_opt = ltu::get_terminal_profile(argv[1]);
    if (!terminal_profile_opt.has_value()) {
        ltu::usage_error();
    }
    auto terminal_profile = terminal_profile_opt.value(); 
    /* Scan Desktop Entries */
    std::map<std::string, ltu::desktop_entry_t> entries;
    for (const std::filesystem::path &path : ltu::desktop_file_paths()) {
        if (!std::filesystem::is_directory(path)) continue; 
        for (auto dir_entry : std::filesystem::directory_iterator(path)) {
            if (!dir_entry.is_regular_file() || dir_entry.path().extension() != ".desktop") continue; 
            std::optional<ltu::desktop_entry_t> parse_result = ltu::parse_desktop_file(dir_entry.path());
            if (!parse_result.has_value()) {
                continue;
            }

            if (entries.find(parse_result.value().name) == entries.end())
                entries[parse_result.value().name] = parse_result.value();
        }
    }
    std::vector<std::string> menu_items = {" Exit"};
    for (std::string entry : entries
        | std::views::keys
        | std::ranges::to<std::vector> ()) {
        menu_items.emplace_back(entry);
    }
    /* Select and Run */
    int selected_idx = menu::select(menu_items);
    if (1 > selected_idx) std::exit(0); 
    ltu::desktop_entry_t selected_entry = entries[menu_items[selected_idx]];
    std::exit(ltu::run_desktop_entry(selected_entry, terminal_profile));
}
