#pragma once

#include <string>
#include <optional>
#include <filesystem>
#include <terminal.hpp>

namespace ltu {
    typedef struct {
        std::string name;
        std::string comment;
        std::string exec;
        bool terminal;
        std::filesystem::path path;
    } desktop_entry_t; 

    std::optional<desktop_entry_t> parse_desktop_file(const std::filesystem::path &desktop_file);

    int run_desktop_entry(const desktop_entry_t &desktop_entry, const terminal_profile_t &terminal_profile);
} // namespace ltu
