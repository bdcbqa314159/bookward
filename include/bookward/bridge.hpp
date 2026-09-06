#pragma once

#include <cstdint>
#include <memory>

#include "rust/cxx.h"

namespace bookward {

struct BookRow;  // shared struct, defined by the cxx-generated header

// The cxx bridge: the Rust GUI holds a Log and calls the SAME cmd_* verbs as
// the CLI and TUI — one implementation of the rules, no schema knowledge in
// Rust. Conventions across the bridge: year 0 = "default/current";
// worked as i8: -1 = not a technical book, 0 = not worked, 1 = worked.
class Log {
 public:
  Log();
  ~Log();

  rust::Vec<BookRow> books();
  rust::String add(rust::Str title, rust::Str author, rust::Str edition, std::int64_t year,
                   std::int8_t worked);
  rust::String again(rust::Str id, std::int64_t year);
  rust::String edit(rust::Str id, rust::Str title, rust::Str author, rust::Str edition,
                    std::int8_t worked);
  rust::String move_year(rust::Str id, std::int64_t from, std::int64_t to);
  rust::String remove_book(rust::Str id);
  rust::String remove_reading(rust::Str id, std::int64_t year);
  rust::String report(std::int64_t year, rust::Str out_dir);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

std::unique_ptr<Log> open_log_ptr();

}  // namespace bookward
