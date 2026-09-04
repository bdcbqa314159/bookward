#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <vector>

#include "bookward/commands.hpp"

namespace bookward {

namespace {

std::int64_t this_year() {
  return static_cast<int>(std::chrono::year_month_day{
      std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())}
                              .year());
}

std::string lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

Book must_get(dataward::Store& store, const std::string& id) {
  auto book = store.get<Book>(id);
  if (!book) throw std::runtime_error("no book '" + id + "'");
  return *book;
}

std::string next_id(dataward::Store& store) {
  // ponytail: lexicographic MAX works while ids are zero-padded to 4 digits;
  // revisit at book 9999.
  auto last = store.query_string("SELECT MAX(\"id\") FROM \"Book\"");
  const int n = last ? std::stoi(last->substr(3)) + 1 : 1;
  char buf[16];
  std::snprintf(buf, sizeof buf, "bk-%04d", n);
  return buf;
}

std::string one_line(const Book& b) {
  std::string line = b.id + "  " + b.title;
  if (!b.author.empty()) line += " — " + b.author;
  if (!b.edition.empty()) line += " (" + b.edition + ")";
  line += "  [" + std::to_string(b.year) + "]";
  if (b.worked) line += *b.worked ? "  worked" : "  not worked";
  return line;
}

std::string lines_for(const std::vector<Book>& books) {
  if (books.empty()) return "(no books)";
  std::string out;
  for (const auto& b : books) {
    if (!out.empty()) out += '\n';
    out += one_line(b);
  }
  return out;
}

}  // namespace

std::string cmd_add(dataward::Store& store, const std::string& title, const std::string& author,
                    const std::string& edition, std::optional<std::int64_t> year,
                    std::optional<bool> worked) {
  if (title.empty()) throw std::runtime_error("add needs a title");

  Book b;
  b.id = next_id(store);
  b.title = title;
  b.author = author;
  b.edition = edition;
  b.year = year.value_or(this_year());
  b.worked = worked;
  store.put(b);
  return "added " + one_line(b);
}

std::string cmd_find(dataward::Store& store, const std::string& query) {
  if (query.empty()) throw std::runtime_error("find needs a search term");
  const auto needle = lower(query);
  auto books = store.where<Book>("1=1 ORDER BY \"id\"");
  std::erase_if(books, [&](const Book& b) {
    return lower(b.title).find(needle) == std::string::npos &&
           lower(b.author).find(needle) == std::string::npos;
  });
  return lines_for(books);
}

std::string cmd_list(dataward::Store& store, std::int64_t year) {
  auto books = year == 0 ? store.where<Book>("1=1 ORDER BY \"year\" DESC, \"id\"")
                         : store.where<Book>("\"year\" = :b0 ORDER BY \"id\"", year);
  return lines_for(books);
}

std::string cmd_worked(dataward::Store& store, const std::string& id, bool worked) {
  auto b = must_get(store, id);
  b.worked = worked;
  store.put(b);
  return one_line(b);
}

std::string cmd_year(dataward::Store& store, const std::string& id, std::int64_t year) {
  if (year < 1000 || year > 9999) throw std::runtime_error("year must be 4 digits");
  auto b = must_get(store, id);
  b.year = year;
  store.put(b);
  return one_line(b);
}

std::string cmd_remove(dataward::Store& store, const std::string& id) {
  auto b = must_get(store, id);
  store.remove<Book>(id);
  return "removed " + one_line(b);
}

}  // namespace bookward
