#include <algorithm>
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

Element detail_pane(const bookward::Book& b) {
  Elements lines;
  lines.push_back(text(b.title) | bold);
  if (!b.author.empty()) lines.push_back(text("by " + b.author));
  if (!b.edition.empty()) lines.push_back(text(b.edition));
  lines.push_back(text("read in " + std::to_string(b.year)));
  if (b.worked)
    lines.push_back(text(*b.worked ? "worked through" : "not worked through") | underlined);
  lines.push_back(separator());
  lines.push_back(text("pdf: " + b.id + ".pdf") | dim);
  return vbox(lines) | border | flex;
}

// What the bottom input line is currently collecting.
enum class Mode {
  Browse,
  AddTitle,
  AddAuthor,
  AddEdition,
  AddYear,
  AddWorked,
  EditYear,
  RemoveConfirm
};

}  // namespace

int main() {
  auto store = bookward::open_log();

  std::vector<bookward::Book> books;
  std::vector<std::string> entries;
  int selected = 0;
  auto reload = [&] {
    books = store.where<bookward::Book>("1=1 ORDER BY \"year\" DESC, \"id\"");
    entries.clear();
    for (const auto& b : books)
      entries.push_back(b.id + "  " + b.title + "  [" + std::to_string(b.year) + "]");
    if (selected >= static_cast<int>(entries.size()))
      selected = entries.empty() ? 0 : static_cast<int>(entries.size()) - 1;
  };
  reload();

  int tab = 0;
  int sort_col = 0;
  std::int64_t year_filter = 0;

  Mode mode = Mode::Browse;
  std::string buffer;
  std::string add_title, add_author, add_edition;
  std::optional<std::int64_t> add_year;
  std::string status_msg = "ready";

  auto prompt_label = [&]() -> std::string {
    switch (mode) {
      case Mode::AddTitle:
        return "title: ";
      case Mode::AddAuthor:
        return "author (empty ok): ";
      case Mode::AddEdition:
        return "edition (empty ok): ";
      case Mode::AddYear:
        return "year (empty = this year): ";
      case Mode::AddWorked:
        return "worked? y/n (empty = not technical): ";
      case Mode::EditYear:
        return "set year to: ";
      case Mode::RemoveConfirm:
        return "remove — type the id to confirm: ";
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
        case Mode::AddTitle:
          add_title = buffer;
          mode = Mode::AddAuthor;
          buffer.clear();
          return;
        case Mode::AddAuthor:
          add_author = buffer;
          mode = Mode::AddEdition;
          buffer.clear();
          return;
        case Mode::AddEdition:
          add_edition = buffer;
          mode = Mode::AddYear;
          buffer.clear();
          return;
        case Mode::AddYear:
          add_year = buffer.empty() ? std::nullopt : std::optional(std::stoll(buffer));
          mode = Mode::AddWorked;
          buffer.clear();
          return;
        case Mode::AddWorked: {
          std::optional<bool> worked;
          if (buffer == "y") worked = true;
          if (buffer == "n") worked = false;
          status_msg =
              bookward::cmd_add(store, add_title, add_author, add_edition, add_year, worked);
          break;
        }
        case Mode::EditYear:
          status_msg = bookward::cmd_year(store, selected_id(), std::stoll(buffer));
          break;
        case Mode::RemoveConfirm:
          if (buffer == selected_id()) {
            status_msg = bookward::cmd_remove(store, buffer);
          } else {
            status_msg = "id mismatch — not removed";
          }
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
    auto rows = bookward::table_rows(books, sort_col, year_filter);
    std::vector<std::vector<std::string>> cells;
    std::vector<std::string> header(bookward::kTableColumns.begin(), bookward::kTableColumns.end());
    header[static_cast<std::size_t>(sort_col)] += " ↓";
    cells.push_back(header);
    cells.insert(cells.end(), rows.begin(), rows.end());
    auto table = Table(cells);
    table.SelectRow(0).Decorate(bold);
    table.SelectAll().SeparatorVertical(LIGHT);
    const std::string filter_label = year_filter == 0 ? "all" : std::to_string(year_filter);
    return vbox({text(" year: " + filter_label + "   [s] sort column  [f] cycle year ") | dim,
                 table.Render() | vscroll_indicator | frame | flex});
  });

  auto stats_view = Renderer([&] {
    const auto counts = bookward::year_counts(books);
    std::int64_t max_count = 1;
    for (const auto& [y, c] : counts) max_count = std::max(max_count, c.first);
    Elements lines;
    lines.push_back(text(std::to_string(books.size()) + " books total") | bold);
    lines.push_back(separator());
    for (const auto& [y, c] : counts) {
      lines.push_back(hbox(
          {text(std::to_string(y) + " "),
           gauge(static_cast<float>(c.first) / static_cast<float>(max_count)) | flex,
           text(" " + std::to_string(c.first) + " (" + std::to_string(c.second) + " worked)")}));
    }
    return vbox(lines) | border | flex;
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
               text(tab == 0 ? "[a]dd [w]orked [y]ear [x] remove [r]eport [q]uit " : "[q]uit ") |
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
    if (tab == 1 && e == Event::Character('f')) {
      // Cycle: all -> each year present (newest first) -> all.
      std::vector<std::int64_t> years;
      for (const auto& [y, c] : bookward::year_counts(books)) years.push_back(y);
      if (years.empty()) {
        year_filter = 0;
      } else if (year_filter == 0) {
        year_filter = years.front();
      } else {
        auto it = std::find(years.begin(), years.end(), year_filter);
        year_filter = (it == years.end() || std::next(it) == years.end()) ? 0 : *std::next(it);
      }
    }
    if (tab == 0) {
      if (e == Event::Character('a')) mode = Mode::AddTitle;
      if (!books.empty()) {
        if (e == Event::Character('w')) {
          try {
            const auto& b = books[static_cast<std::size_t>(selected)];
            status_msg = bookward::cmd_worked(store, b.id, !(b.worked && *b.worked));
          } catch (const std::exception& ex) {
            status_msg = ex.what();
          }
          reload();
        }
        if (e == Event::Character('y')) mode = Mode::EditYear;
        if (e == Event::Character('x')) mode = Mode::RemoveConfirm;
        if (e == Event::Character('r')) {
          try {
            status_msg = bookward::cmd_report(store, 0, "", /*compile=*/true);
#ifdef __APPLE__
            if (status_msg.rfind(".pdf") == status_msg.size() - 4)
              std::system(("open \"" + status_msg.substr(6) + "\"").c_str());
#endif
          } catch (const std::exception& ex) {
            status_msg = ex.what();
          }
        }
      }
    }
    return e.is_character();
  });

  screen.Loop(app);
  return 0;
}
