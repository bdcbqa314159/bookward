#include <cstdio>
#include <exception>
#include <optional>
#include <string>
#include <vector>

#include "bookward/commands.hpp"

namespace {

constexpr const char* kUsage = R"(bookward — a reading catalog (ids name the PDFs)
  bookward add <title> [--author A] [--edition E] [--year Y] [--worked yes|no]
  bookward again <id> [--year Y]        log a re-read of an existing book
  bookward edit <id> [--title T] [--author A] [--edition E]
  bookward find <text>                  search titles and authors
  bookward list [--year Y]
  bookward worked <id> yes|no
  bookward year <id> [from] <to>        move a reading to another year
  bookward remove <id> [--year Y]       forget one reading, or the whole book
  bookward report [year]                LaTeX/PDF catalog (no year = everything)
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

std::optional<bool> parse_worked(const std::optional<std::string>& s) {
  if (!s) return std::nullopt;
  if (*s == "yes") return true;
  if (*s == "no") return false;
  throw std::runtime_error("--worked takes yes or no");
}

std::optional<std::int64_t> parse_year_flag(std::vector<std::string>& args) {
  if (auto y = take_flag(args, "--year")) return std::stoll(*y);
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
      const auto edition = take_flag(args, "--edition").value_or("");
      const auto year = parse_year_flag(args);
      const auto worked = parse_worked(take_flag(args, "--worked"));
      if (args.size() != 1)
        throw std::runtime_error("usage: add <title> [--author] [--edition] [--year] [--worked]");
      out = bookward::cmd_add(store, args[0], author, edition, year, worked);
    } else if (cmd == "again") {
      const auto year = parse_year_flag(args);
      if (args.size() != 1) throw std::runtime_error("usage: again <id> [--year Y]");
      out = bookward::cmd_again(store, args[0], year);
    } else if (cmd == "edit") {
      const auto title = take_flag(args, "--title");
      const auto author = take_flag(args, "--author");
      const auto edition = take_flag(args, "--edition");
      if (args.size() != 1 || (!title && !author && !edition))
        throw std::runtime_error("usage: edit <id> [--title] [--author] [--edition]");
      out = bookward::cmd_edit(store, args[0], title, author, edition);
    } else if (cmd == "find") {
      if (args.size() != 1) throw std::runtime_error("usage: find <text>");
      out = bookward::cmd_find(store, args[0]);
    } else if (cmd == "list") {
      out = bookward::cmd_list(store, parse_year_flag(args).value_or(0));
    } else if (cmd == "worked") {
      if (args.size() != 2) throw std::runtime_error("usage: worked <id> yes|no");
      out = bookward::cmd_worked(store, args[0], *parse_worked(args[1]));
    } else if (cmd == "year") {
      if (args.size() == 2) {
        out = bookward::cmd_year(store, args[0], std::nullopt, std::stoll(args[1]));
      } else if (args.size() == 3) {
        out = bookward::cmd_year(store, args[0], std::stoll(args[1]), std::stoll(args[2]));
      } else {
        throw std::runtime_error("usage: year <id> [from] <to>");
      }
    } else if (cmd == "remove") {
      const auto year = parse_year_flag(args);
      if (args.size() != 1) throw std::runtime_error("usage: remove <id> [--year Y]");
      out = bookward::cmd_remove(store, args[0], year);
    } else if (cmd == "report") {
      const auto out_dir = take_flag(args, "--out").value_or("");
      const std::int64_t year = args.empty() ? 0 : std::stoll(args[0]);
      out = bookward::cmd_report(store, year, out_dir, /*compile=*/true);
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
