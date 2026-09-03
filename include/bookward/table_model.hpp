#pragma once

#include <array>
#include <string>
#include <vector>

#include "bookward/book.hpp"

namespace bookward {

// The raw database view: every Book column, stringified exactly as stored
// (ISO dates, NULLs as empty cells). Pure — the TUI renders it, tests drive it.
inline constexpr std::array<const char*, 10> kTableColumns = {
    "id",      "title",    "author",       "pages",  "status",
    "started", "finished", "current_page", "rating", "notes"};

// Rows filtered by status ("" = all) and sorted by column index
// (numeric columns compare numerically, the rest as text).
std::vector<std::vector<std::string>> table_rows(std::vector<Book> books, int sort_col,
                                                 const std::string& status_filter);

}  // namespace bookward
