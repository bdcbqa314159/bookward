#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "bookward/book.hpp"

namespace bookward {

// One year's reading aggregates — the single owner of the numbers the PDF
// report prints and the TUI stats pane renders.
struct YearStats {
  std::int64_t finished = 0;
  std::int64_t pages = 0;
  std::int64_t rated = 0;
  std::int64_t rating_sum = 0;
  // Convention: a book's pages count toward its finish month.
  std::array<std::int64_t, 12> pages_by_month{};

  double avg_rating() const {
    return rated > 0 ? static_cast<double>(rating_sum) / static_cast<double>(rated) : 0.0;
  }
};

YearStats year_stats(const std::vector<Book>& books, std::int64_t year);

}  // namespace bookward
