#pragma once

#include <cstdint>
#include <dataward/store.hpp>
#include <optional>
#include <string>

namespace bookward {

// A catalog of books read. One Book = one physical book = one PDF on the
// archive drive, named by the GENERATED id (bk-0001, ...). The id never
// changes — re-reading a book adds a Reading row, not a second Book.
struct Book {
  std::string id;  // generated, primary key, names the PDF
  std::string title;
  std::string author;
  std::string edition;
  std::optional<bool> worked;  // technical books: worked through or not; absent otherwise
};
BOOST_DESCRIBE_STRUCT(Book, (), (id, title, author, edition, worked))

// One "I read this book in this year". id = "<book_id>-<year>", so logging the
// same book twice in one year is naturally idempotent (REPLACE hits the same row).
struct Reading {
  std::string id;
  std::string book_id;  // FK by convention
  std::int64_t year = 0;
};
BOOST_DESCRIBE_STRUCT(Reading, (), (id, book_id, year))

// Opens the catalog: $BOOKWARD_DB if set, else ~/.bookward.db.
dataward::Store open_log();

}  // namespace bookward
