#include <cstdio>
#include <exception>
#include <optional>
#include <string>
#include <vector>

#include "bookward/commands.hpp"

namespace {

constexpr const char* kUsage = R"(bookward — a reading log
  bookward add <id> <title> [--author A] [--pages N]
  bookward progress <id> <page>
  bookward finish <id> [--rating 1..5]
  bookward shelve <id>
  bookward list [--status reading|finished|shelved]
)";

// Pulls "--flag value" out of args (and erases it); nullopt when absent.
std::optional<std::string> take_flag(std::vector<std::string>& args, const std::string& flag) {
  for (std::size_t i = 0; i + 1 < args.size(); ++i) {
    if (args[i] == flag) {
      std::string value = args[i + 1];
      args.erase(args.begin() + static_cast<std::ptrdiff_t>(i),
                 args.begin() + static_cast<std::ptrdiff_t>(i) + 2);
      return value;
    }
  }
  return std::nullopt;
}

}  // namespace

int main(int argc, char** argv) {
  std::vector<std::string> args(argv + 1, argv + argc);
  if (args.empty()) {
    std::fputs(kUsage, stderr);
    return 1;
  }
  const std::string cmd = args.front();
  args.erase(args.begin());

  try {
    auto store = bookward::open_log();
    std::string out;
    if (cmd == "add") {
      const auto author = take_flag(args, "--author").value_or("");
      const auto pages = std::stoll(take_flag(args, "--pages").value_or("0"));
      if (args.size() != 2)
        throw std::runtime_error("usage: add <id> <title> [--author] [--pages]");
      out = bookward::cmd_add(store, args[0], args[1], author, pages);
    } else if (cmd == "progress") {
      if (args.size() != 2) throw std::runtime_error("usage: progress <id> <page>");
      out = bookward::cmd_progress(store, args[0], std::stoll(args[1]));
    } else if (cmd == "finish") {
      std::optional<std::int64_t> rating;
      if (auto r = take_flag(args, "--rating")) rating = std::stoll(*r);
      if (args.size() != 1) throw std::runtime_error("usage: finish <id> [--rating]");
      out = bookward::cmd_finish(store, args[0], rating);
    } else if (cmd == "shelve") {
      if (args.size() != 1) throw std::runtime_error("usage: shelve <id>");
      out = bookward::cmd_shelve(store, args[0]);
    } else if (cmd == "list") {
      out = bookward::cmd_list(store, take_flag(args, "--status").value_or(""));
    } else {
      std::fputs(kUsage, stderr);
      return 1;
    }
    std::puts(out.c_str());
    return 0;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "bookward: %s\n", e.what());
    return 1;
  }
}
