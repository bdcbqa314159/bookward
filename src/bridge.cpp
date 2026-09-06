#include "bookward/bridge.hpp"

#include "bookward/commands.hpp"

namespace bookward {

struct Log::Impl {
  dataward::Store store;
};

Log::Log() : impl_(new Impl{open_log()}) {}
Log::~Log() = default;

rust::String Log::list(std::int64_t year) { return cmd_list(impl_->store, year); }

std::unique_ptr<Log> open_log_ptr() { return std::make_unique<Log>(); }

}  // namespace bookward
