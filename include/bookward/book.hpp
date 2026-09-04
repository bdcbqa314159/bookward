#pragma once

#include <cstdint>
#include <dataward/store.hpp>
#include <optional>
#include <string>

namespace bookward {

// A catalog of books read — not a progress tracker. The id is GENERATED
// (bk-0001, bk-0002, ...) and doubles as the filename of the book's PDF on the
// archive drive; the database is the index connecting titles to files.
struct Book {
  std::string id;  // generated, primary key, names the PDF
  std::string title;
  std::string author;
  std::string edition;
  std::int64_t year = 0;       // year read; books from before the list began backfill here
  std::optional<bool> worked;  // technical books: worked through or not; absent otherwise
};
BOOST_DESCRIBE_STRUCT(Book, (), (id, title, author, edition, year, worked))

// Opens the catalog: $BOOKWARD_DB if set, else ~/.bookward.db.
dataward::Store open_log();

}  // namespace bookward
