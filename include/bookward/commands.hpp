#pragma once

#include <optional>
#include <string>
#include <vector>

#include "bookward/book.hpp"

namespace bookward {

// Each command returns the line(s) to print; invalid input throws
// std::runtime_error with the message the CLI shows (exit 1).

// Generates the next bk-NNNN id, logs a first Reading (year: nullopt = current).
std::string cmd_add(dataward::Store& store, const std::string& title, const std::string& author,
                    const std::string& edition, std::optional<std::int64_t> year,
                    std::optional<bool> worked);
// Logs another reading of an existing book (re-read).
std::string cmd_again(dataward::Store& store, const std::string& id,
                      std::optional<std::int64_t> year);
// Corrects any of the text fields; nullopt = leave unchanged.
std::string cmd_edit(dataward::Store& store, const std::string& id,
                     std::optional<std::string> title, std::optional<std::string> author,
                     std::optional<std::string> edition);
// Moves one reading to another year. from = nullopt is allowed only when the
// book has exactly one reading.
std::string cmd_year(dataward::Store& store, const std::string& id,
                     std::optional<std::int64_t> from, std::int64_t to);
// nullopt clears the field back to "not a technical book".
std::string cmd_worked(dataward::Store& store, const std::string& id, std::optional<bool> worked);
// year given: forget just that reading. year nullopt: remove book + readings.
std::string cmd_remove(dataward::Store& store, const std::string& id,
                       std::optional<std::int64_t> year);
// Case-insensitive substring search over title and author.
std::string cmd_find(dataward::Store& store, const std::string& query);
// year 0 = all books; otherwise books read in that year.
std::string cmd_list(dataward::Store& store, std::int64_t year);
// LaTeX catalog. year 0 = every year, grouped, newest first; a book read in
// two years appears under both.
std::string cmd_report(dataward::Store& store, std::int64_t year, const std::string& out_dir,
                       bool compile);

// Shared lookups (also used by the TUI).
std::vector<std::int64_t> years_of(dataward::Store& store, const std::string& book_id);

}  // namespace bookward
