#include "bookward/stats.hpp"

namespace bookward {

YearCounts year_counts(const std::vector<Book>& books) {
  YearCounts counts;
  for (const auto& b : books) {
    auto& [total, worked] = counts[b.year];
    ++total;
    if (b.worked && *b.worked) ++worked;
  }
  return counts;
}

}  // namespace bookward
