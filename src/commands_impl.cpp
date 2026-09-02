#include <chrono>
#include <stdexcept>
#include <vector>

#include "bookward/commands.hpp"

namespace bookward {

namespace {

std::chrono::year_month_day today() {
  return std::chrono::year_month_day{
      std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())};
}

Book must_get(dataward::Store& store, const std::string& id) {
  auto book = store.get<Book>(id);
  if (!book) throw std::runtime_error("no book '" + id + "'");
  return *book;
}

void check_status(const std::string& status) {
  if (status != "reading" && status != "finished" && status != "shelved")
    throw std::runtime_error("status must be reading|finished|shelved, got '" + status + "'");
}

std::string one_line(const Book& b) {
  std::string line = b.id + "  " + b.title + " (" + b.author + ") — " + b.status;
  if (b.status == "reading")
    line += ", page " + std::to_string(b.current_page) + "/" + std::to_string(b.pages);
  if (b.rating) line += " [" + std::to_string(*b.rating) + "/5]";
  return line;
}

}  // namespace

std::string cmd_add(dataward::Store& store, const std::string& id, const std::string& title,
                    const std::string& author, std::int64_t pages) {
  if (id.empty() || title.empty()) throw std::runtime_error("add needs an id and a title");
  if (pages <= 0) throw std::runtime_error("--pages must be positive");
  if (store.get<Book>(id)) throw std::runtime_error("book '" + id + "' already exists");

  Book b;
  b.id = id;
  b.title = title;
  b.author = author;
  b.pages = pages;
  b.started = today();
  store.put(b);
  return "added " + one_line(b);
}

std::string cmd_progress(dataward::Store& store, const std::string& id, std::int64_t page) {
  auto b = must_get(store, id);
  if (b.status != "reading")
    throw std::runtime_error("'" + id + "' is " + b.status + ", not reading");
  if (page <= 0 || page > b.pages)
    throw std::runtime_error("page must be in 1.." + std::to_string(b.pages));
  b.current_page = page;
  store.put(b);
  return one_line(b);
}

std::string cmd_finish(dataward::Store& store, const std::string& id,
                       std::optional<std::int64_t> rating) {
  if (rating && (*rating < 1 || *rating > 5)) throw std::runtime_error("--rating must be 1..5");
  auto b = must_get(store, id);
  b.status = "finished";
  b.finished = today();
  b.current_page = b.pages;
  if (rating) b.rating = rating;
  store.put(b);
  return "finished " + one_line(b);
}

std::string cmd_shelve(dataward::Store& store, const std::string& id) {
  auto b = must_get(store, id);
  b.status = "shelved";
  store.put(b);
  return "shelved " + one_line(b);
}

std::string cmd_list(dataward::Store& store, const std::string& status_filter) {
  std::vector<Book> books;
  if (status_filter.empty()) {
    books = store.where<Book>("1=1 ORDER BY \"status\", \"id\"");
  } else {
    check_status(status_filter);
    books = store.where<Book>("\"status\" = :b0 ORDER BY \"id\"", status_filter);
  }
  if (books.empty()) return "(no books)";
  std::string out;
  for (const auto& b : books) {
    if (!out.empty()) out += '\n';
    out += one_line(b);
  }
  return out;
}

}  // namespace bookward
