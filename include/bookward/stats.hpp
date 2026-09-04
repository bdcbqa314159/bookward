#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "bookward/book.hpp"

namespace bookward {

// Books per year (and how many of those were worked through), newest first.
using YearCounts = std::map<std::int64_t, std::pair<std::int64_t, std::int64_t>, std::greater<>>;

YearCounts year_counts(const std::vector<Book>& books);

}  // namespace bookward
