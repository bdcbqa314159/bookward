#pragma once

#include <optional>
#include <string>

#include "bookward/book.hpp"

namespace bookward {

// Each command returns the line(s) to print; invalid input throws
// std::runtime_error with the message the CLI shows (exit 1).

// Generates the next bk-NNNN id and returns "added bk-NNNN  Title ...".
// year: nullopt = current year. worked: nullopt = not a technical book.
std::string cmd_add(dataward::Store& store, const std::string& title, const std::string& author,
                    const std::string& edition, std::optional<std::int64_t> year,
                    std::optional<bool> worked);
// Case-insensitive substring search over title and author.
std::string cmd_find(dataward::Store& store, const std::string& query);
// year 0 = all years.
std::string cmd_list(dataward::Store& store, std::int64_t year);
std::string cmd_worked(dataward::Store& store, const std::string& id, bool worked);
// Manual correction of the year an existing entry was read.
std::string cmd_year(dataward::Store& store, const std::string& id, std::int64_t year);
std::string cmd_remove(dataward::Store& store, const std::string& id);
// LaTeX catalog. year 0 = every year, grouped, newest first.
std::string cmd_report(dataward::Store& store, std::int64_t year, const std::string& out_dir,
                       bool compile);

}  // namespace bookward
