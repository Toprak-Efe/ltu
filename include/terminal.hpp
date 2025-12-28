#pragma once

#include <string>
#include <map>
#include <optional>

namespace ltu {

    typedef struct {
        std::string binary;
        std::string flag;
    } terminal_profile_t;

    const std::map<std::string, terminal_profile_t> g_known_terminals = {
        {"gnome-terminal", {"gnome-terminal", "--"}},
        {"konsole",        {"konsole", "-e"}},
        {"kitty",          {"kitty", "-e"}},
        {"alacritty",      {"alacritty", "-e"}},
        {"wezterm",        {"wezterm", "start --"}},
        {"terminator",     {"terminator", "-e"}},
        {"tilix",          {"tilix", "-e"}},
        {"xterm",          {"xterm", "-e"}},
        {"urxvt",          {"urxvt", "-e"}},        
        {"xfce4-terminal", {"xfce4-terminal", "-x"}} 
    };

    std::optional<terminal_profile_t> get_terminal_profile();

} // namespace ltu 

