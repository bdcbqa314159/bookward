#pragma once

#include <array>
#include <string>
#include <vector>

#include "bookward/book.hpp"

namespace bookward {

// The raw database view: every Book column, stringified exactly as stored
// (NULL worked as an empty cell). Pure — the TUI renders it, tests drive it.
inline constexpr std::array<const char*, 6> kTableColumns = {"id",      "title", "author",
                                                             "edition", "year",  "worked"};

// Rows filtered by year (0 = all) and sorted by column index
// (year compares numerically, the rest as text).
std::vector<std::vector<std::string>> table_rows(std::vector<Book> books, int sort_col,
                                                 std::int64_t year_filter);

}  // namespace bookward
