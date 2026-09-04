#include <cstdlib>

#include "bookward/book.hpp"

namespace bookward {

dataward::Store open_log() {
  std::string path;
  if (const char* env = std::getenv("BOOKWARD_DB"); env != nullptr && *env != '\0') {
    path = env;
  } else {
    const char* home = std::getenv("HOME");
#ifdef _WIN32
    if (home == nullptr) home = std::getenv("USERPROFILE");
#endif
    path = std::string(home != nullptr ? home : ".") + "/.bookward.db";
  }
  auto store = dataward::Store::sqlite(path);
  store.ensure<Book>();
  store.ensure<Reading>();
  return store;
}

}  // namespace bookward
