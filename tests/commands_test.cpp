#include "bookward/commands.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>

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

dataward::Store fresh(const char* name) {
  const auto path = std::filesystem::path(testing::TempDir()) / name;
  std::filesystem::remove(path);
  auto store = dataward::Store::sqlite(path.string());
  store.ensure<bookward::Book>();
  return store;
}

TEST(Commands, AddThenListAndDuplicateRejected) {
  auto store = fresh("cmd_add.db");
  bookward::cmd_add(store, "dune", "Dune", "Frank Herbert", 412);
  EXPECT_THROW(bookward::cmd_add(store, "dune", "Dune", "", 412), std::runtime_error);
  EXPECT_THROW(bookward::cmd_add(store, "x", "X", "", 0), std::runtime_error);

  auto out = bookward::cmd_list(store, "");
  EXPECT_NE(out.find("dune"), std::string::npos);
  EXPECT_NE(out.find("page 0/412"), std::string::npos);
}

TEST(Commands, ProgressBoundsAndStatus) {
  auto store = fresh("cmd_progress.db");
  bookward::cmd_add(store, "dune", "Dune", "", 412);
  EXPECT_THROW(bookward::cmd_progress(store, "nope", 10), std::runtime_error);
  EXPECT_THROW(bookward::cmd_progress(store, "dune", 0), std::runtime_error);
  EXPECT_THROW(bookward::cmd_progress(store, "dune", 413), std::runtime_error);

  bookward::cmd_progress(store, "dune", 120);
  EXPECT_EQ(store.get<bookward::Book>("dune")->current_page, 120);

  bookward::cmd_shelve(store, "dune");
  EXPECT_THROW(bookward::cmd_progress(store, "dune", 130), std::runtime_error);
}

TEST(Commands, FinishSetsEverything) {
  auto store = fresh("cmd_finish.db");
  bookward::cmd_add(store, "dune", "Dune", "", 412);
  EXPECT_THROW(bookward::cmd_finish(store, "dune", 6), std::runtime_error);

  bookward::cmd_finish(store, "dune", 5);
  auto b = store.get<bookward::Book>("dune");
  EXPECT_EQ(b->status, "finished");
  EXPECT_EQ(b->current_page, 412);
  EXPECT_EQ(b->rating, 5);
  EXPECT_TRUE(b->finished.has_value());
}

TEST(Commands, ListFiltersByStatus) {
  auto store = fresh("cmd_list.db");
  bookward::cmd_add(store, "a", "A", "", 100);
  bookward::cmd_add(store, "b", "B", "", 100);
  bookward::cmd_finish(store, "b", std::nullopt);

  auto reading = bookward::cmd_list(store, "reading");
  EXPECT_NE(reading.find("a  A"), std::string::npos);
  EXPECT_EQ(reading.find("b  B"), std::string::npos);

  EXPECT_THROW(bookward::cmd_list(store, "bogus"), std::runtime_error);
  EXPECT_EQ(bookward::cmd_list(store, "shelved"), "(no books)");
}

}  // namespace
