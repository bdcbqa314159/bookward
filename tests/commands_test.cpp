#include "bookward/commands.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "bookward/stats.hpp"
#include "bookward/table_model.hpp"

namespace {

void set_env(const char* name, const std::string& value) {
#ifdef _WIN32
  _putenv_s(name, value.c_str());
#else
  setenv(name, value.c_str(), 1);
#endif
}

dataward::Store fresh(const char* name) {
  const auto path = std::filesystem::path(testing::TempDir()) / name;
  std::filesystem::remove(path);
  auto store = dataward::Store::sqlite(path.string());
  store.ensure<bookward::Book>();
  return store;
}

// The consumer proof: a Book round-trips through dataward via open_log().
TEST(Bookward, BookRoundTripsThroughDataward) {
  const auto path = std::filesystem::path(testing::TempDir()) / "bookward_smoke.db";
  std::filesystem::remove(path);
  set_env("BOOKWARD_DB", path.string());

  auto store = bookward::open_log();
  bookward::cmd_add(store, "Dune", "Frank Herbert", "1st", 2024, std::nullopt);

  auto back = store.get<bookward::Book>("bk-0001");
  ASSERT_TRUE(back.has_value());
  EXPECT_EQ(back->title, "Dune");
  EXPECT_EQ(back->edition, "1st");
  EXPECT_EQ(back->year, 2024);
  EXPECT_EQ(back->worked, std::nullopt);
}

TEST(Commands, IdsAreSequentialAndSurviveRemoval) {
  auto store = fresh("cmd_ids.db");
  bookward::cmd_add(store, "A", "", "", 2026, std::nullopt);
  bookward::cmd_add(store, "B", "", "", 2026, std::nullopt);
  bookward::cmd_remove(store, "bk-0002");
  auto out = bookward::cmd_add(store, "C", "", "", 2026, std::nullopt);
  EXPECT_NE(out.find("bk-0002"), std::string::npos)
      << "next id comes from MAX(id)+1; removing the last book recycles its id";
  EXPECT_THROW(bookward::cmd_remove(store, "bk-9999"), std::runtime_error);
  EXPECT_THROW(bookward::cmd_add(store, "", "", "", std::nullopt, std::nullopt),
               std::runtime_error);
}

TEST(Commands, FindMatchesTitleAndAuthorCaseInsensitive) {
  auto store = fresh("cmd_find.db");
  bookward::cmd_add(store, "Dune", "Frank Herbert", "", 2024, std::nullopt);
  bookward::cmd_add(store, "Stochastic Calculus", "Shreve", "2nd", 2025, true);

  EXPECT_NE(bookward::cmd_find(store, "dune").find("bk-0001"), std::string::npos);
  EXPECT_NE(bookward::cmd_find(store, "shreve").find("bk-0002"), std::string::npos);
  EXPECT_EQ(bookward::cmd_find(store, "tolkien"), "(no books)");
  EXPECT_THROW(bookward::cmd_find(store, ""), std::runtime_error);
}

TEST(Commands, ListFiltersByYearAndWorkedToggles) {
  auto store = fresh("cmd_list.db");
  bookward::cmd_add(store, "Old", "", "", 2021, std::nullopt);
  bookward::cmd_add(store, "New", "", "", 2026, false);

  EXPECT_EQ(bookward::cmd_list(store, 2021).find("New"), std::string::npos);
  EXPECT_NE(bookward::cmd_list(store, 0).find("Old"), std::string::npos);

  bookward::cmd_worked(store, "bk-0002", true);
  EXPECT_EQ(store.get<bookward::Book>("bk-0002")->worked, true);
  EXPECT_NE(bookward::cmd_list(store, 2026).find("worked"), std::string::npos);

  // Manual year correction: seeded wrong, fixed after the fact.
  bookward::cmd_year(store, "bk-0002", 2024);
  EXPECT_EQ(store.get<bookward::Book>("bk-0002")->year, 2024);
  EXPECT_THROW(bookward::cmd_year(store, "bk-0002", 26), std::runtime_error);
  EXPECT_THROW(bookward::cmd_year(store, "bk-9999", 2024), std::runtime_error);
}

TEST(Report, CatalogGroupsByYearNewestFirst) {
  auto store = fresh("cmd_report.db");
  bookward::cmd_add(store, "Tom & Jerry 100% Guide", "A_Uthor", "3rd", 2021, std::nullopt);
  bookward::cmd_add(store, "Recent", "", "", 2026, true);

  const auto dir = std::filesystem::path(testing::TempDir()) / "bookward_report";
  bookward::cmd_report(store, 0, dir.string(), /*compile=*/false);

  std::ifstream in(dir / "reading-catalog.tex");
  std::string doc((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  EXPECT_NE(doc.find("Tom \\& Jerry 100\\% Guide"), std::string::npos) << "escaping";
  EXPECT_NE(doc.find("A\\_Uthor"), std::string::npos);
  EXPECT_NE(doc.find("2 books"), std::string::npos);
  EXPECT_LT(doc.find("{2026"), doc.find("{2021")) << "newest year first";
  EXPECT_NE(doc.find("bk-0002 & Recent &  &  & yes"), std::string::npos);
  EXPECT_NE(doc.find("\\end{document}"), std::string::npos);
}

TEST(Report, SingleYearReportOnlyHasThatYear) {
  auto store = fresh("cmd_report_year.db");
  bookward::cmd_add(store, "Old", "", "", 2021, std::nullopt);
  bookward::cmd_add(store, "New", "", "", 2026, std::nullopt);

  const auto dir = std::filesystem::path(testing::TempDir()) / "bookward_report_year";
  bookward::cmd_report(store, 2026, dir.string(), /*compile=*/false);
  std::ifstream in(dir / "reading-2026.tex");
  std::string doc((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  EXPECT_NE(doc.find("New"), std::string::npos);
  EXPECT_EQ(doc.find("Old"), std::string::npos);
}

TEST(TableModel, FilterSortAndEmptyWorked) {
  auto store = fresh("table_model.db");
  bookward::cmd_add(store, "Beta", "", "", 2026, true);
  bookward::cmd_add(store, "Alpha", "", "", 2021, std::nullopt);

  auto rows = bookward::table_rows(store.all<bookward::Book>(), 1, 0);  // sort by title
  ASSERT_EQ(rows.size(), 2u);
  EXPECT_EQ(rows[0][1], "Alpha");
  EXPECT_EQ(rows[0][5], "") << "absent worked renders empty";
  EXPECT_EQ(rows[1][5], "yes");

  rows = bookward::table_rows(store.all<bookward::Book>(), 4, 2021);  // year filter
  ASSERT_EQ(rows.size(), 1u);
  EXPECT_EQ(rows[0][1], "Alpha");
}

TEST(Stats, CountsPerYearNewestFirst) {
  auto store = fresh("stats.db");
  bookward::cmd_add(store, "A", "", "", 2021, std::nullopt);
  bookward::cmd_add(store, "B", "", "", 2026, true);
  bookward::cmd_add(store, "C", "", "", 2026, false);

  auto counts = bookward::year_counts(store.all<bookward::Book>());
  ASSERT_EQ(counts.size(), 2u);
  EXPECT_EQ(counts.begin()->first, 2026) << "newest first";
  EXPECT_EQ(counts[2026].first, 2);
  EXPECT_EQ(counts[2026].second, 1) << "one of the 2026 books was worked";
  EXPECT_EQ(counts[2021].first, 1);
}

}  // namespace
