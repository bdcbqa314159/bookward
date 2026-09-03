#include "bookward/table_model.hpp"

#include <algorithm>
#include <cstdio>

namespace bookward {

namespace {

std::string iso(const std::optional<std::chrono::year_month_day>& d) {
  if (!d) return "";
  char buf[16];
  std::snprintf(buf, sizeof buf, "%04d-%02u-%02u", static_cast<int>(d->year()),
                static_cast<unsigned>(d->month()), static_cast<unsigned>(d->day()));
  return buf;
}

std::vector<std::string> to_row(const Book& b) {
  return {b.id,
          b.title,
          b.author,
          std::to_string(b.pages),
          b.status,
          iso(b.started),
          iso(b.finished),
          std::to_string(b.current_page),
          b.rating ? std::to_string(*b.rating) : "",
          b.notes.value_or("")};
}

bool numeric_column(int col) { return col == 3 || col == 7 || col == 8; }

}  // namespace

std::vector<std::vector<std::string>> table_rows(std::vector<Book> books, int sort_col,
                                                 const std::string& status_filter) {
  if (!status_filter.empty())
    std::erase_if(books, [&](const Book& b) { return b.status != status_filter; });

  std::vector<std::vector<std::string>> rows;
  rows.reserve(books.size());
  for (const auto& b : books) rows.push_back(to_row(b));

  const auto col = static_cast<std::size_t>(sort_col);
  std::stable_sort(rows.begin(), rows.end(), [&](const auto& a, const auto& b) {
    if (numeric_column(sort_col)) {
      // Empty cells (NULL rating) sort last.
      if (a[col].empty() != b[col].empty()) return b[col].empty();
      if (a[col].empty()) return false;
      return std::stoll(a[col]) < std::stoll(b[col]);
    }
    return a[col] < b[col];
  });
  return rows;
}

}  // namespace bookward
