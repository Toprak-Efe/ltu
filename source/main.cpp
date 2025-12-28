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
}; // namespace ltu

int main (int argc, char *argv[]) {
    /* Fetch Terminal Info */
    auto terminal_profile_opt = ltu::get_terminal_profile();
    if (!terminal_profile_opt.has_value()) {
        std::cerr << "Unable to detect terminal command through $TERMCMD, exiting.\n";
        return 1;
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
    if (1 > selected_idx) return 0;
    ltu::desktop_entry_t selected_entry = entries[menu_items[selected_idx]];
    return ltu::run_desktop_entry(selected_entry, terminal_profile);
}
