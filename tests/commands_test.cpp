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
  store.ensure<bookward::Reading>();
  return store;
}

// The consumer proof: both tables round-trip through dataward via open_log().
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
  EXPECT_EQ(bookward::years_of(store, "bk-0001"), std::vector<std::int64_t>{2024});
}

TEST(Commands, EditFixesFieldsInPlace) {
  auto store = fresh("cmd_edit.db");
  bookward::cmd_add(store, "Suenos de acero", "Cortazar", "", 2023, std::nullopt);

  // The n -> ñ workflow: id and readings survive, text is corrected.
  bookward::cmd_edit(store, "bk-0001", "Sueños de acero", "Cortázar", std::nullopt);
  auto b = store.get<bookward::Book>("bk-0001");
  EXPECT_EQ(b->title, "Sueños de acero");
  EXPECT_EQ(b->author, "Cortázar");
  EXPECT_EQ(b->edition, "");
  EXPECT_EQ(bookward::years_of(store, "bk-0001"), std::vector<std::int64_t>{2023});

  EXPECT_THROW(bookward::cmd_edit(store, "bk-0001", std::string(""), std::nullopt, std::nullopt),
               std::runtime_error)
      << "cannot edit the title away";
  EXPECT_THROW(bookward::cmd_edit(store, "bk-9999", std::string("X"), std::nullopt, std::nullopt),
               std::runtime_error);
}

TEST(Commands, RereadKeepsIdAndCountsInBothYears) {
  auto store = fresh("cmd_again.db");
  bookward::cmd_add(store, "El Quijote", "Cervantes", "", 2021, std::nullopt);
  auto out = bookward::cmd_again(store, "bk-0001", 2026);

  EXPECT_NE(out.find("bk-0001"), std::string::npos) << "same id";
  EXPECT_EQ(bookward::years_of(store, "bk-0001"), (std::vector<std::int64_t>{2021, 2026}));
  EXPECT_EQ(store.all<bookward::Book>().size(), 1u) << "still one book, one PDF";

  // Logging the same year twice is idempotent.
  bookward::cmd_again(store, "bk-0001", 2026);
  EXPECT_EQ(bookward::years_of(store, "bk-0001").size(), 2u);
}

TEST(Commands, YearMovesOneReading) {
  auto store = fresh("cmd_year.db");
  bookward::cmd_add(store, "A", "", "", 2023, std::nullopt);

  bookward::cmd_year(store, "bk-0001", std::nullopt, 2022);  // single reading: from optional
  EXPECT_EQ(bookward::years_of(store, "bk-0001"), std::vector<std::int64_t>{2022});

  bookward::cmd_again(store, "bk-0001", 2026);
  EXPECT_THROW(bookward::cmd_year(store, "bk-0001", std::nullopt, 2020), std::runtime_error)
      << "ambiguous with two readings";
  bookward::cmd_year(store, "bk-0001", 2022, 2021);
  EXPECT_EQ(bookward::years_of(store, "bk-0001"), (std::vector<std::int64_t>{2021, 2026}));
}

TEST(Commands, RemoveOneReadingOrWholeBook) {
  auto store = fresh("cmd_remove.db");
  bookward::cmd_add(store, "A", "", "", 2021, std::nullopt);
  bookward::cmd_again(store, "bk-0001", 2026);

  bookward::cmd_remove(store, "bk-0001", 2021);
  EXPECT_EQ(bookward::years_of(store, "bk-0001"), std::vector<std::int64_t>{2026});
  EXPECT_THROW(bookward::cmd_remove(store, "bk-0001", 2026), std::runtime_error)
      << "last reading: must remove the book";

  bookward::cmd_remove(store, "bk-0001", std::nullopt);
  EXPECT_EQ(store.all<bookward::Book>().size(), 0u);
  EXPECT_TRUE(store.all<bookward::Reading>().empty()) << "no orphaned readings";
}

TEST(Commands, FindAndListShowAllYears) {
  auto store = fresh("cmd_find.db");
  bookward::cmd_add(store, "Dune", "Frank Herbert", "", 2021, std::nullopt);
  bookward::cmd_again(store, "bk-0001", 2026);
  bookward::cmd_add(store, "Other", "", "", 2026, true);

  auto hit = bookward::cmd_find(store, "dune");
  EXPECT_NE(hit.find("[2021, 2026]"), std::string::npos);

  EXPECT_EQ(bookward::cmd_list(store, 2021).find("Other"), std::string::npos);
  EXPECT_NE(bookward::cmd_list(store, 2026).find("Dune"), std::string::npos)
      << "re-read appears in the later year too";
}

TEST(Report, RereadAppearsUnderBothYears) {
  auto store = fresh("cmd_report.db");
  bookward::cmd_add(store, "Tom & Jerry 100% Guide", "A_Uthor", "3rd", 2021, std::nullopt);
  bookward::cmd_again(store, "bk-0001", 2026);
  bookward::cmd_add(store, "Recent", "", "", 2026, true);

  const auto dir = std::filesystem::path(testing::TempDir()) / "bookward_report";
  bookward::cmd_report(store, 0, dir.string(), /*compile=*/false);

  std::ifstream in(dir / "reading-catalog.tex");
  std::string doc((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  EXPECT_NE(doc.find("Tom \\& Jerry 100\\% Guide"), std::string::npos) << "escaping";
  EXPECT_NE(doc.find("3 readings, 2 distinct books"), std::string::npos);
  EXPECT_LT(doc.find("{2026"), doc.find("{2021")) << "newest year first";
  // The re-read book appears in both year sections.
  const auto first = doc.find("Tom \\&");
  EXPECT_NE(doc.find("Tom \\&", first + 1), std::string::npos);
  EXPECT_NE(doc.find("\\end{document}"), std::string::npos);
}

TEST(TableModel, YearsAggregateAndFilter) {
  auto store = fresh("table_model.db");
  bookward::cmd_add(store, "Beta", "", "", 2026, true);
  bookward::cmd_add(store, "Alpha", "", "", 2021, std::nullopt);
  bookward::cmd_again(store, "bk-0002", 2026);

  auto books = store.all<bookward::Book>();
  auto readings = store.all<bookward::Reading>();

  auto rows = bookward::table_rows(books, readings, 1, 0);  // sort by title
  ASSERT_EQ(rows.size(), 2u);
  EXPECT_EQ(rows[0][1], "Alpha");
  EXPECT_EQ(rows[0][4], "2021 2026");
  EXPECT_EQ(rows[1][5], "yes");

  rows = bookward::table_rows(books, readings, 0, 2021);
  ASSERT_EQ(rows.size(), 1u);
  EXPECT_EQ(rows[0][1], "Alpha");
}

TEST(Stats, CountsReadingsPerYear) {
  auto store = fresh("stats.db");
  bookward::cmd_add(store, "A", "", "", 2021, std::nullopt);
  bookward::cmd_again(store, "bk-0001", 2026);
  bookward::cmd_add(store, "B", "", "", 2026, true);

  auto counts = bookward::year_counts(store.all<bookward::Book>(), store.all<bookward::Reading>());
  ASSERT_EQ(counts.size(), 2u);
  EXPECT_EQ(counts.begin()->first, 2026) << "newest first";
  EXPECT_EQ(counts[2026].first, 2) << "re-read counts in 2026 too";
  EXPECT_EQ(counts[2026].second, 1);
  EXPECT_EQ(counts[2021].first, 1);
}

}  // namespace
