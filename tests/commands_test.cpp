#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>

#include "bookward/book.hpp"

namespace {

void set_env(const char* name, const std::string& value) {
#ifdef _WIN32
  _putenv_s(name, value.c_str());
#else
  setenv(name, value.c_str(), 1);
#endif
}

// The consumer proof: a Book round-trips through dataward.
TEST(Bookward, BookRoundTripsThroughDataward) {
  const auto path = std::filesystem::path(testing::TempDir()) / "bookward_smoke.db";
  std::filesystem::remove(path);
  set_env("BOOKWARD_DB", path.string());

  auto store = bookward::open_log();
  bookward::Book dune;
  dune.id = "dune";
  dune.title = "Dune";
  dune.author = "Frank Herbert";
  dune.pages = 412;
  dune.started = std::chrono::year{2026} / std::chrono::month{9} / std::chrono::day{1};
  store.put(dune);

  auto back = store.get<bookward::Book>("dune");
  ASSERT_TRUE(back.has_value());
  EXPECT_EQ(back->title, "Dune");
  EXPECT_EQ(back->status, "reading");
  EXPECT_EQ(back->current_page, 0);
  EXPECT_EQ(back->rating, std::nullopt);
  EXPECT_EQ(back->started, dune.started);
}

}  // namespace
