#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>

namespace menu {

    int select(const std::vector<std::string>& entries) {
        using namespace ftxui;
        if (entries.empty()) return -1;

        int selected_in_filtered = 0;
        int final_result = -1;

        std::vector<int> filtered_indices(entries.size());
        std::iota(filtered_indices.begin(), filtered_indices.end(), 0);

        std::string search_query;
        std::vector<std::string> filtered_entries = entries;
        auto screen = ScreenInteractive::Fullscreen();
        InputOption input_option;
        input_option.on_change = [&] {
            filtered_entries.clear();
            filtered_indices.clear();
            selected_in_filtered = 0; 
            std::string query_lower = search_query;
            std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
            for (size_t i = 0; i < entries.size(); ++i) {
                std::string entry_lower = entries[i];
                std::transform(entry_lower.begin(), entry_lower.end(), entry_lower.begin(), ::tolower);
                if (entry_lower.find(query_lower) != std::string::npos) {
                    filtered_entries.push_back(entries[i]);
                    filtered_indices.push_back(i);
                }
            }
        };
        input_option.transform = [](InputState state) {
            return state.element; 
        };
        auto input = Input(&search_query, "Search...", input_option);
        MenuOption menu_option;
        menu_option.on_enter = [&] {
            if (!filtered_indices.empty()) {
                final_result = filtered_indices[selected_in_filtered];
                screen.ExitLoopClosure()();
            }
        };
        menu_option.entries_option.transform = [](const EntryState& state) {
            Element e;
            if (state.active) {
                e = hbox({
                    text("> "), 
                    text(state.label) | bold
                });
            } else {
                e = hbox({
                    text("  "), 
                    text(state.label)
                });
            }
            return e;
        };
        auto menu = Menu(&filtered_entries, &selected_in_filtered, menu_option);
        auto renderer = Renderer([&] {
                return vbox({
                        hbox(text(""), input->Render()),
                        separator(),
                        filtered_entries.empty() 
                        ? text("No matches") 
                        : menu->Render() | vscroll_indicator | frame | flex
                        });
                });
        renderer |= CatchEvent([&](Event event) {
            if (event == Event::Escape) {
                final_result = 0; 
                screen.ExitLoopClosure()();
                return true;
            }
            if (event == Event::ArrowUp || event == Event::ArrowDown || 
                event == Event::PageUp  || event == Event::PageDown ||
                event == Event::Home    || event == Event::End) {
                return menu->OnEvent(event);
            }
            if (event == Event::Return) {
                return menu->OnEvent(event);
            }
            return input->OnEvent(event);
        });

        screen.Loop(renderer);
        return final_result;
    }
} // namespace menu
