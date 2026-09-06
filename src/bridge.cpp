#include "bookward/bridge.hpp"

#include <optional>
#include <string>

#include "bookward-gui/src/lib.rs.h"
#include "bookward/commands.hpp"

namespace bookward {

namespace {

std::optional<std::int64_t> opt_year(std::int64_t year) {
  return year == 0 ? std::nullopt : std::optional(year);
}

std::optional<bool> opt_worked(std::int8_t worked) {
  if (worked < 0) return std::nullopt;
  return worked != 0;
}

}  // namespace

struct Log::Impl {
  dataward::Store store;
};

Log::Log() : impl_(new Impl{open_log()}) {}
Log::~Log() = default;

rust::Vec<BookRow> Log::books() {
  rust::Vec<BookRow> rows;
  for (const auto& b : impl_->store.where<Book>("1=1 ORDER BY \"id\"")) {
    BookRow row;
    row.id = b.id;
    row.title = b.title;
    row.author = b.author;
    row.edition = b.edition;
    row.worked = b.worked ? (*b.worked ? 1 : 0) : -1;
    for (auto y : years_of(impl_->store, b.id)) row.years.push_back(y);
    rows.push_back(std::move(row));
  }
  return rows;
}

rust::String Log::add(rust::Str title, rust::Str author, rust::Str edition, std::int64_t year,
                      std::int8_t worked) {
  return cmd_add(impl_->store, std::string(title), std::string(author), std::string(edition),
                 opt_year(year), opt_worked(worked));
}

rust::String Log::again(rust::Str id, std::int64_t year) {
  return cmd_again(impl_->store, std::string(id), opt_year(year));
}

rust::String Log::edit(rust::Str id, rust::Str title, rust::Str author, rust::Str edition,
                       std::int8_t worked) {
  cmd_edit(impl_->store, std::string(id), std::string(title), std::string(author),
           std::string(edition));
  return cmd_worked(impl_->store, std::string(id), opt_worked(worked));
}

rust::String Log::move_year(rust::Str id, std::int64_t from, std::int64_t to) {
  return cmd_year(impl_->store, std::string(id), opt_year(from), to);
}

rust::String Log::remove_book(rust::Str id) {
  return cmd_remove(impl_->store, std::string(id), std::nullopt);
}

rust::String Log::remove_reading(rust::Str id, std::int64_t year) {
  return cmd_remove(impl_->store, std::string(id), year);
}

rust::String Log::report(std::int64_t year, rust::Str out_dir) {
  return cmd_report(impl_->store, year, std::string(out_dir), /*compile=*/true);
}

std::unique_ptr<Log> open_log_ptr() { return std::make_unique<Log>(); }

}  // namespace bookward
