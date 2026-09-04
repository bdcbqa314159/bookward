#include <chrono>
#include <cstdlib>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <string>
#include <vector>

#include "bookward/commands.hpp"
#include "bookward/stats.hpp"
#include "bookward/table_model.hpp"

using namespace ftxui;

namespace {

constexpr std::array<const char*, 12> kMonths = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

std::int64_t current_year() {
  return static_cast<int>(std::chrono::year_month_day{
      std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())}
                              .year());
}

Element detail_pane(const bookward::Book& b) {
  Elements lines;
  lines.push_back(text(b.title) | bold);
  if (!b.author.empty()) lines.push_back(text("by " + b.author));
  lines.push_back(text(b.status + ", " + std::to_string(b.pages) + " pages"));
  if (b.status == "reading") {
    const float frac =
        b.pages > 0 ? static_cast<float>(b.current_page) / static_cast<float>(b.pages) : 0.f;
    lines.push_back(hbox({text("page " + std::to_string(b.current_page) + " "), gauge(frac) | flex,
                          text(" " + std::to_string(int(frac * 100)) + "%")}));
  }
  if (b.rating) lines.push_back(text("rating " + std::to_string(*b.rating) + "/5"));
  if (b.notes) lines.push_back(paragraph(*b.notes) | dim);
  return vbox(lines) | border | flex;
}

// What the bottom input line is currently collecting.
enum class Mode { Browse, Page, Rating, Year, StatsYear, AddId, AddTitle, AddAuthor, AddPages };

}  // namespace

int main() {
  auto store = bookward::open_log();

  std::vector<bookward::Book> books;
  std::vector<std::string> entries;
  int selected = 0;
  auto reload = [&] {
    books = store.where<bookward::Book>("1=1 ORDER BY \"status\", \"id\"");
    entries.clear();
    for (const auto& b : books) entries.push_back(b.id + "  " + b.title + "  [" + b.status + "]");
    if (selected >= static_cast<int>(entries.size()))
      selected = entries.empty() ? 0 : static_cast<int>(entries.size()) - 1;
  };
  reload();

  int tab = 0;
  int sort_col = 0;
  std::size_t filter_idx = 0;
  const std::vector<std::string> filters = {"", "reading", "finished", "shelved"};
  std::int64_t stats_year = current_year();

  Mode mode = Mode::Browse;
  std::string buffer;                         // the live input text
  std::string add_id, add_title, add_author;  // collected add-form steps
  std::string status_msg = "ready";

  auto prompt_label = [&]() -> std::string {
    switch (mode) {
      case Mode::Page:
        return "page: ";
      case Mode::Rating:
        return "rating 1-5 (empty = none): ";
      case Mode::Year:
        return "report year: ";
      case Mode::StatsYear:
        return "year: ";
      case Mode::AddId:
        return "new book id: ";
      case Mode::AddTitle:
        return "title: ";
      case Mode::AddAuthor:
        return "author (empty ok): ";
      case Mode::AddPages:
        return "pages: ";
      default:
        return "";
    }
  };

  auto selected_id = [&]() -> std::string {
    return books.empty() ? "" : books[static_cast<std::size_t>(selected)].id;
  };

  auto apply = [&] {
    try {
      switch (mode) {
        case Mode::Page:
          status_msg = bookward::cmd_progress(store, selected_id(), std::stoll(buffer));
          break;
        case Mode::Rating: {
          std::optional<std::int64_t> rating;
          if (!buffer.empty()) rating = std::stoll(buffer);
          status_msg = bookward::cmd_finish(store, selected_id(), rating);
          break;
        }
        case Mode::Year: {
          const auto year = buffer.empty() ? current_year() : std::stoll(buffer);
          status_msg = bookward::cmd_report(store, year, "", /*compile=*/true);
#ifdef __APPLE__
          if (status_msg.rfind(".pdf") == status_msg.size() - 4)
            std::system(("open \"" + status_msg.substr(6) + "\"").c_str());
#endif
          break;
        }
        case Mode::StatsYear:
          stats_year = buffer.empty() ? current_year() : std::stoll(buffer);
          status_msg = "showing " + std::to_string(stats_year);
          break;
        case Mode::AddId:
          add_id = buffer;
          mode = Mode::AddTitle;
          buffer.clear();
          return;
        case Mode::AddTitle:
          add_title = buffer;
          mode = Mode::AddAuthor;
          buffer.clear();
          return;
        case Mode::AddAuthor:
          add_author = buffer;
          mode = Mode::AddPages;
          buffer.clear();
          return;
        case Mode::AddPages:
          status_msg = bookward::cmd_add(store, add_id, add_title, add_author, std::stoll(buffer));
          break;
        default:
          break;
      }
    } catch (const std::exception& e) {
      status_msg = e.what();
    }
    mode = Mode::Browse;
    buffer.clear();
    reload();
  };

  auto menu = Menu(&entries, &selected);
  auto books_view = Renderer(menu, [&] {
    Element right = books.empty() ? text("(no books — press a to add one)") | center | border | flex
                                  : detail_pane(books[static_cast<std::size_t>(selected)]);
    return hbox(
        {menu->Render() | vscroll_indicator | frame | border | size(WIDTH, LESS_THAN, 48), right});
  });

  auto table_view = Renderer([&] {
    auto rows = bookward::table_rows(books, sort_col, filters[filter_idx]);
    std::vector<std::vector<std::string>> cells;
    std::vector<std::string> header(bookward::kTableColumns.begin(), bookward::kTableColumns.end());
    header[static_cast<std::size_t>(sort_col)] += " ↓";
    cells.push_back(header);
    cells.insert(cells.end(), rows.begin(), rows.end());
    auto table = Table(cells);
    table.SelectRow(0).Decorate(bold);
    table.SelectAll().SeparatorVertical(LIGHT);
    const std::string filter_label = filters[filter_idx].empty() ? "all" : filters[filter_idx];
    return vbox({text(" filter: " + filter_label + "   [s] sort column  [f] filter ") | dim,
                 table.Render() | vscroll_indicator | frame | flex});
  });

  auto stats_view = Renderer([&] {
    const auto s = bookward::year_stats(books, stats_year);
    Elements lines;
    lines.push_back(text(std::to_string(stats_year) + "  —  " + std::to_string(s.finished) +
                         " finished, " + std::to_string(s.pages) + " pages") |
                    bold);
    if (s.rated > 0) {
      char avg[16];
      std::snprintf(avg, sizeof avg, "%.1f", s.avg_rating());
      lines.push_back(text("average rating " + std::string(avg) + "/5"));
    }
    lines.push_back(separator());
    std::int64_t max_month = 1;
    for (auto p : s.pages_by_month) max_month = std::max(max_month, p);
    for (int m = 0; m < 12; ++m) {
      const auto pages = s.pages_by_month[static_cast<std::size_t>(m)];
      lines.push_back(hbox({text(std::string(kMonths[static_cast<std::size_t>(m)]) + " "),
                            gauge(static_cast<float>(pages) / static_cast<float>(max_month)) | flex,
                            text(" " + std::to_string(pages)) | size(WIDTH, GREATER_THAN, 6)}));
    }
    return vbox({text(" [y] change year ") | dim, vbox(lines) | border | flex});
  });

  auto body = Container::Tab({books_view, table_view, stats_view}, &tab);
  auto screen = ScreenInteractive::Fullscreen();

  auto app = Renderer(body, [&] {
    Element bottom =
        mode == Mode::Browse
            ? text(" " + status_msg + " ") | dim
            : hbox({text(" " + prompt_label()) | bold, text(buffer), text("▌") | blink});
    return vbox(
        {hbox({text(" bookward ") | bold | inverted,
               text(std::string("  [1] Books") + (tab == 0 ? "*" : "") + " [2] Table" +
                    (tab == 1 ? "*" : "") + " [3] Stats" + (tab == 2 ? "*" : "") + "  "),
               filler(),
               text(tab == 0 ? "[a]dd [p]rogress [F]inish [v]shelve [r]eport [q]uit " : "[q]uit ") |
                   dim}),
         body->Render() | flex, bottom});
  });

  app = CatchEvent(app, [&](Event e) {
    if (mode != Mode::Browse) {  // input line owns the keyboard
      if (e == Event::Return) {
        apply();
      } else if (e == Event::Escape) {
        mode = Mode::Browse;
        buffer.clear();
        status_msg = "cancelled";
      } else if (e == Event::Backspace) {
        if (!buffer.empty()) buffer.pop_back();
      } else if (e.is_character()) {
        buffer += e.character();
      }
      return true;
    }
    if (e == Event::Character('q')) {
      screen.Exit();
      return true;
    }
    if (e == Event::Character('1')) tab = 0;
    if (e == Event::Character('2')) tab = 1;
    if (e == Event::Character('3')) tab = 2;
    if (tab == 1 && e == Event::Character('s'))
      sort_col = (sort_col + 1) % static_cast<int>(bookward::kTableColumns.size());
    if (tab == 1 && e == Event::Character('f')) filter_idx = (filter_idx + 1) % filters.size();
    if (tab == 2 && e == Event::Character('y')) mode = Mode::StatsYear;
    if (tab == 0) {
      if (e == Event::Character('a')) mode = Mode::AddId;
      if (!books.empty()) {
        if (e == Event::Character('p')) mode = Mode::Page;
        if (e == Event::Character('F')) mode = Mode::Rating;
        if (e == Event::Character('v')) {
          try {
            status_msg = bookward::cmd_shelve(store, selected_id());
          } catch (const std::exception& ex) {
            status_msg = ex.what();
          }
          reload();
        }
        if (e == Event::Character('r')) mode = Mode::Year;
      }
    }
    return e.is_character();
  });

  screen.Loop(app);
  return 0;
}
