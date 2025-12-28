#include <terminal.hpp>
#include <iostream>

std::optional<ltu::terminal_profile_t> ltu::get_terminal_profile(const char *term_arg) {
    std::string term_cmd(term_arg);
    size_t idx = term_cmd.find(' ');
    term_cmd = term_cmd.substr(0, idx);
    if (g_known_terminals.find(term_cmd) == g_known_terminals.end()) return {};
    return g_known_terminals.at(term_cmd);
}
