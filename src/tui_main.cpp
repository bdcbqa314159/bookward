#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <string>
#include <vector>

#include "bookward/commands.hpp"
#include "bookward/table_model.hpp"

using namespace ftxui;

namespace {

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

}  // namespace

int main() {
  auto store = bookward::open_log();
  const auto books = store.where<bookward::Book>("1=1 ORDER BY \"status\", \"id\"");

  int tab = 0;
  int selected = 0;
  int sort_col = 0;
  std::size_t filter_idx = 0;
  const std::vector<std::string> filters = {"", "reading", "finished", "shelved"};

  std::vector<std::string> entries;
  entries.reserve(books.size());
  for (const auto& b : books) entries.push_back(b.id + "  " + b.title + "  [" + b.status + "]");

  auto menu = Menu(&entries, &selected);
  auto books_view = Renderer(menu, [&] {
    Element right = books.empty()
                        ? text("(no books — add some with the CLI)") | center | border | flex
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

  auto body = Container::Tab({books_view, table_view}, &tab);
  auto screen = ScreenInteractive::Fullscreen();

  auto app = Renderer(body, [&] {
    return vbox({hbox({text(" bookward ") | bold | inverted,
                       text(tab == 0 ? "  [1] Books* [2] Table  " : "  [1] Books [2] Table*  "),
                       filler(), text("[q] quit ") | dim}),
                 body->Render() | flex});
  });
  app = CatchEvent(app, [&](Event e) {
    if (e == Event::Character('q')) {
      screen.Exit();
      return true;
    }
    if (e == Event::Character('1')) tab = 0;
    if (e == Event::Character('2')) tab = 1;
    if (tab == 1 && e == Event::Character('s'))
      sort_col = (sort_col + 1) % static_cast<int>(bookward::kTableColumns.size());
    if (tab == 1 && e == Event::Character('f')) filter_idx = (filter_idx + 1) % filters.size();
    return e == Event::Character('1') || e == Event::Character('2') || e == Event::Character('s') ||
           e == Event::Character('f');
  });

  screen.Loop(app);
  return 0;
}
