#include "bookward/table_model.hpp"

#include <algorithm>

namespace bookward {

namespace {

std::vector<std::string> to_row(const Book& b) {
  return {b.id,
          b.title,
          b.author,
          b.edition,
          std::to_string(b.year),
          b.worked ? (*b.worked ? "yes" : "no") : ""};
}

}  // namespace

std::vector<std::vector<std::string>> table_rows(std::vector<Book> books, int sort_col,
                                                 std::int64_t year_filter) {
  if (year_filter != 0) std::erase_if(books, [&](const Book& b) { return b.year != year_filter; });

  std::vector<std::vector<std::string>> rows;
  rows.reserve(books.size());
  for (const auto& b : books) rows.push_back(to_row(b));

  const auto col = static_cast<std::size_t>(sort_col);
  std::stable_sort(rows.begin(), rows.end(), [&](const auto& a, const auto& b) {
    if (sort_col == 4) return std::stoll(a[col]) < std::stoll(b[col]);  // year
    return a[col] < b[col];
  });
  return rows;
}

}  // namespace bookward
