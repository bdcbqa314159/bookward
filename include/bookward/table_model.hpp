#pragma once

#include <array>
#include <string>
#include <vector>

#include "bookward/book.hpp"

namespace bookward {

// The raw database view: Book columns plus the aggregated years of its
// readings. Pure — the TUI renders it, tests drive it.
inline constexpr std::array<const char*, 6> kTableColumns = {"id",      "title", "author",
                                                             "edition", "years", "worked"};

// Rows filtered by year (0 = all) and sorted by column index (text compare;
// the years cell is "2021 2026" so text order == numeric order).
std::vector<std::vector<std::string>> table_rows(const std::vector<Book>& books,
                                                 const std::vector<Reading>& readings, int sort_col,
                                                 std::int64_t year_filter);

}  // namespace bookward
