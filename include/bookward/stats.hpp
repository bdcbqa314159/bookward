#pragma once

#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>

#include "bookward/book.hpp"

namespace bookward {

// Readings per year (and how many were of worked-through books), newest first.
// A book read in two years counts in both.
using YearCounts = std::map<std::int64_t, std::pair<std::int64_t, std::int64_t>, std::greater<>>;

YearCounts year_counts(const std::vector<Book>& books, const std::vector<Reading>& readings);

}  // namespace bookward
