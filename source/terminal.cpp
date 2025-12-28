#include <terminal.hpp>

std::optional<ltu::terminal_profile_t> ltu::get_terminal_profile() {
    const char* term_cmd = std::getenv("TERMCMD"); 
    if (!term_cmd) return {};
    std::string term_cmd_str(term_cmd);
    size_t idx = term_cmd_str.find(' ');
    term_cmd_str = term_cmd_str.substr(0, idx);
    if (g_known_terminals.find(term_cmd_str) == g_known_terminals.end()) return {};
    return g_known_terminals.at(term_cmd_str);
}
