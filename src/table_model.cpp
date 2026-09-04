#include "bookward/table_model.hpp"

#include <algorithm>
#include <unordered_map>

namespace bookward {

std::vector<std::vector<std::string>> table_rows(const std::vector<Book>& books,
                                                 const std::vector<Reading>& readings, int sort_col,
                                                 std::int64_t year_filter) {
  std::unordered_map<std::string, std::vector<std::int64_t>> years;
  for (const auto& r : readings) years[r.book_id].push_back(r.year);
  for (auto& [id, ys] : years) std::sort(ys.begin(), ys.end());

  std::vector<std::vector<std::string>> rows;
  rows.reserve(books.size());
  for (const auto& b : books) {
    const auto& ys = years[b.id];
    if (year_filter != 0 && std::find(ys.begin(), ys.end(), year_filter) == ys.end()) continue;
    std::string year_cell;
    for (auto y : ys) {
      if (!year_cell.empty()) year_cell += ' ';
      year_cell += std::to_string(y);
    }
    rows.push_back({b.id, b.title, b.author, b.edition, year_cell,
                    b.worked ? (*b.worked ? "yes" : "no") : ""});
  }

  const auto col = static_cast<std::size_t>(sort_col);
  std::stable_sort(rows.begin(), rows.end(),
                   [&](const auto& a, const auto& b) { return a[col] < b[col]; });
  return rows;
}

}  // namespace bookward
