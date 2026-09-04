#include "bookward/stats.hpp"

namespace bookward {

YearStats year_stats(const std::vector<Book>& books, std::int64_t year) {
  YearStats s;
  for (const auto& b : books) {
    if (!b.finished || static_cast<std::int64_t>(static_cast<int>(b.finished->year())) != year)
      continue;
    ++s.finished;
    s.pages += b.pages;
    s.pages_by_month[static_cast<unsigned>(b.finished->month()) - 1] += b.pages;
    if (b.rating) {
      s.rating_sum += *b.rating;
      ++s.rated;
    }
  }
  return s;
}

}  // namespace bookward
