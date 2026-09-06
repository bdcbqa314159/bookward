#pragma once

#include <cstdint>
#include <memory>

#include "rust/cxx.h"

namespace bookward {

// The cxx bridge: the Rust GUI holds a Log and calls the SAME cmd_* verbs as
// the CLI and TUI — one implementation of the rules, no schema knowledge in Rust.
class Log {
 public:
  Log();
  ~Log();
  rust::String list(std::int64_t year);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

std::unique_ptr<Log> open_log_ptr();

}  // namespace bookward
