#pragma once

#include <chrono>
#include <cstdint>
#include <dataward/store.hpp>
#include <optional>
#include <string>

namespace bookward {

// The whole schema of bookward. status is one of: reading | finished | shelved
// (validated in commands, stored as text).
struct Book {
  std::string id;  // slug, primary key
  std::string title;
  std::string author;
  std::int64_t pages = 0;
  std::string status = "reading";
  std::optional<std::chrono::year_month_day> started;
  std::optional<std::chrono::year_month_day> finished;
  std::int64_t current_page = 0;
  std::optional<std::int64_t> rating;  // 1-5
  std::optional<std::string> notes;
};
BOOST_DESCRIBE_STRUCT(Book, (),
                      (id, title, author, pages, status, started, finished, current_page, rating,
                       notes))

// Opens the reading log: $BOOKWARD_DB if set, else ~/.bookward.db.
dataward::Store open_log();

}  // namespace bookward
