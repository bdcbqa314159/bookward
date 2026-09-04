#include "bookward/stats.hpp"

namespace bookward {

YearCounts year_counts(const std::vector<Book>& books, const std::vector<Reading>& readings) {
  std::unordered_map<std::string, bool> worked;
  for (const auto& b : books) worked[b.id] = b.worked && *b.worked;

  YearCounts counts;
  for (const auto& r : readings) {
    auto& [total, worked_count] = counts[r.year];
    ++total;
    if (auto it = worked.find(r.book_id); it != worked.end() && it->second) ++worked_count;
  }
  return counts;
}

}  // namespace bookward
