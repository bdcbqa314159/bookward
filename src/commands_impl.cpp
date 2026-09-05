#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <stdexcept>

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

void check_year(std::int64_t year) {
  if (year < 1000 || year > 9999) throw std::runtime_error("year must be 4 digits");
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

void log_reading(dataward::Store& store, const std::string& book_id, std::int64_t year) {
  check_year(year);
  store.put(Reading{book_id + "-" + std::to_string(year), book_id, year});
}

std::string years_str(const std::vector<std::int64_t>& years) {
  std::string out = "[";
  for (auto y : years) {
    if (out.size() > 1) out += ", ";
    out += std::to_string(y);
  }
  return out + "]";
}

std::string one_line(dataward::Store& store, const Book& b) {
  std::string line = b.id + "  " + b.title;
  if (!b.author.empty()) line += " — " + b.author;
  if (!b.edition.empty()) line += " (" + b.edition + ")";
  line += "  " + years_str(years_of(store, b.id));
  if (b.worked) line += *b.worked ? "  worked" : "  not worked";
  return line;
}

std::string lines_for(dataward::Store& store, const std::vector<Book>& books) {
  if (books.empty()) return "(no books)";
  std::string out;
  for (const auto& b : books) {
    if (!out.empty()) out += '\n';
    out += one_line(store, b);
  }
  return out;
}

}  // namespace

std::vector<std::int64_t> years_of(dataward::Store& store, const std::string& book_id) {
  auto readings = store.where<Reading>("\"book_id\" = :b0 ORDER BY \"year\"", book_id);
  std::vector<std::int64_t> years;
  years.reserve(readings.size());
  for (const auto& r : readings) years.push_back(r.year);
  return years;
}

std::string cmd_add(dataward::Store& store, const std::string& title, const std::string& author,
                    const std::string& edition, std::optional<std::int64_t> year,
                    std::optional<bool> worked) {
  if (title.empty()) throw std::runtime_error("add needs a title");

  Book b;
  b.id = next_id(store);
  b.title = title;
  b.author = author;
  b.edition = edition;
  b.worked = worked;
  auto txn = store.begin();
  store.put(b);
  log_reading(store, b.id, year.value_or(this_year()));
  txn.commit();
  return "added " + one_line(store, b);
}

std::string cmd_again(dataward::Store& store, const std::string& id,
                      std::optional<std::int64_t> year) {
  auto b = must_get(store, id);
  log_reading(store, id, year.value_or(this_year()));
  return one_line(store, b);
}

std::string cmd_edit(dataward::Store& store, const std::string& id,
                     std::optional<std::string> title, std::optional<std::string> author,
                     std::optional<std::string> edition) {
  auto b = must_get(store, id);
  if (title) b.title = *title;
  if (author) b.author = *author;
  if (edition) b.edition = *edition;
  if (b.title.empty()) throw std::runtime_error("a book needs a title");
  store.put(b);
  return one_line(store, b);
}

std::string cmd_year(dataward::Store& store, const std::string& id,
                     std::optional<std::int64_t> from, std::int64_t to) {
  check_year(to);
  auto b = must_get(store, id);
  const auto years = years_of(store, id);
  if (!from) {
    if (years.size() != 1)
      throw std::runtime_error("'" + id + "' was read in " + std::to_string(years.size()) +
                               " years — say which one: year <id> <from> <to>");
    from = years.front();
  }
  if (std::find(years.begin(), years.end(), *from) == years.end())
    throw std::runtime_error("'" + id + "' has no reading in " + std::to_string(*from));
  auto txn = store.begin();
  store.remove<Reading>(id + "-" + std::to_string(*from));
  log_reading(store, id, to);
  txn.commit();
  return one_line(store, b);
}

std::string cmd_worked(dataward::Store& store, const std::string& id, std::optional<bool> worked) {
  auto b = must_get(store, id);
  b.worked = worked;
  store.put(b);
  return one_line(store, b);
}

std::string cmd_remove(dataward::Store& store, const std::string& id,
                       std::optional<std::int64_t> year) {
  auto b = must_get(store, id);
  if (year) {
    const auto years = years_of(store, id);
    if (std::find(years.begin(), years.end(), *year) == years.end())
      throw std::runtime_error("'" + id + "' has no reading in " + std::to_string(*year));
    if (years.size() == 1)
      throw std::runtime_error("that is the only reading — remove the book itself: remove " + id);
    store.remove<Reading>(id + "-" + std::to_string(*year));
    return one_line(store, b);
  }
  const auto line = "removed " + one_line(store, b);
  auto txn = store.begin();
  for (auto y : years_of(store, id)) store.remove<Reading>(id + "-" + std::to_string(y));
  store.remove<Book>(id);
  txn.commit();
  return line;
}

std::string cmd_find(dataward::Store& store, const std::string& query) {
  if (query.empty()) throw std::runtime_error("find needs a search term");
  const auto needle = lower(query);
  auto books = store.where<Book>("1=1 ORDER BY \"id\"");
  std::erase_if(books, [&](const Book& b) {
    return lower(b.title).find(needle) == std::string::npos &&
           lower(b.author).find(needle) == std::string::npos;
  });
  return lines_for(store, books);
}

std::string cmd_list(dataward::Store& store, std::int64_t year) {
  std::vector<Book> books;
  if (year == 0) {
    books = store.where<Book>("1=1 ORDER BY \"id\"");
  } else {
    for (const auto& r : store.where<Reading>("\"year\" = :b0 ORDER BY \"book_id\"", year))
      if (auto b = store.get<Book>(r.book_id)) books.push_back(*b);
  }
  return lines_for(store, books);
}

}  // namespace bookward
