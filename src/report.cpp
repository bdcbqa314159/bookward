#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <unordered_map>
#include <vector>

#include "bookward/commands.hpp"

namespace bookward {

namespace {

// LaTeX-escape user text (titles, authors, editions).
std::string tex(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    switch (c) {
      case '&':
      case '%':
      case '$':
      case '#':
      case '_':
      case '{':
      case '}':
        out += '\\';
        out += c;
        break;
      case '~':
        out += "\\textasciitilde{}";
        break;
      case '^':
        out += "\\textasciicircum{}";
        break;
      case '\\':
        out += "\\textbackslash{}";
        break;
      default:
        out += c;
    }
  }
  return out;
}

// Bibliography entry: Author. <Title>. Edition. (id) — worked
// The italic markers differ per format; everything else is shared.
std::string entry(const Book& b, const std::string& italic_open, const std::string& italic_close,
                  bool escape) {
  auto t = [&](const std::string& s) { return escape ? tex(s) : s; };
  std::string line;
  if (!b.author.empty()) line += t(b.author) + ". ";
  line += italic_open + t(b.title) + italic_close + ".";
  if (!b.edition.empty()) line += " " + t(b.edition) + ".";
  line += " (" + b.id + ")";
  if (b.worked && *b.worked) line += escape ? " --- worked" : " — **worked**";
  return line;
}

}  // namespace

std::string cmd_report(dataward::Store& store, std::int64_t year, const std::string& out_dir,
                       bool compile) {
  auto readings = year == 0 ? store.where<Reading>("1=1 ORDER BY \"year\" DESC, \"book_id\"")
                            : store.where<Reading>("\"year\" = :b0 ORDER BY \"book_id\"", year);

  std::unordered_map<std::string, Book> books;
  for (auto& b : store.all<Book>()) books.emplace(b.id, std::move(b));

  // Group by year, newest first. A book read in two years appears under both.
  std::map<std::int64_t, std::vector<const Book*>, std::greater<>> by_year;
  for (const auto& r : readings)
    if (auto it = books.find(r.book_id); it != books.end()) by_year[r.year].push_back(&it->second);

  const std::string name = year == 0 ? "reading-catalog" : "reading-" + std::to_string(year);
  const std::string title =
      year == 0 ? std::string("Reading Catalog") : "Reading " + std::to_string(year);
  const std::string tally =
      std::to_string(readings.size()) + (readings.size() == 1 ? " reading, " : " readings, ") +
      std::to_string(books.size()) + (books.size() == 1 ? " distinct book" : " distinct books");

  // --- LaTeX: bibliography-style hanging-indent entries, no tables ----------
  std::string doc;
  doc += "\\documentclass[11pt,a4paper]{article}\n";
  doc += "\\usepackage[a4paper,margin=1in]{geometry}\n";
  doc += "\\usepackage{microtype}\n";
  doc += "\\usepackage[hidelinks]{hyperref}\n";
  doc += "\\title{" + title + "}\n\\author{bookward}\n\\date{\\today}\n";
  doc += "\\begin{document}\n\\maketitle\n";
  doc += tally + ".\n";
  for (const auto& [y, group] : by_year) {
    doc += "\\section*{" + std::to_string(y) + " --- " + std::to_string(group.size()) + " book" +
           (group.size() == 1 ? "" : "s") + "}\n";
    for (const Book* b : group)
      doc += "\\noindent\\hangindent=2em " + entry(*b, "\\emph{", "}", true) + "\\par\\medskip\n";
  }
  doc += "\\end{document}\n";

  // --- Markdown twin (renders on GitHub, diffs as the list grows) -----------
  std::string md = "# " + title + "\n\n" + tally + ".\n";
  for (const auto& [y, group] : by_year) {
    md += "\n## " + std::to_string(y) + " — " + std::to_string(group.size()) + " book" +
          (group.size() == 1 ? "" : "s") + "\n\n";
    for (const Book* b : group) md += "- " + entry(*b, "*", "*", false) + "\n";
  }

  const auto dir = std::filesystem::path(out_dir.empty() ? "." : out_dir);
  std::filesystem::create_directories(dir);
  const auto tex_path = dir / (name + ".tex");
  std::ofstream(tex_path) << doc;
  const auto md_path = dir / (name + ".md");
  std::ofstream(md_path) << md;

  if (!compile) return "wrote " + tex_path.string() + " and " + md_path.string();

  const std::string cmd = "latexmk -pdf -interaction=nonstopmode -output-directory=\"" +
                          dir.string() + "\" \"" + tex_path.string() + "\" > /dev/null 2>&1";
  if (std::system(cmd.c_str()) != 0)
    return "wrote " + tex_path.string() + " and " + md_path.string() +
           " (latexmk failed or missing — compile the tex manually)";
  return "wrote " + (dir / (name + ".pdf")).string() + " and " + md_path.string();
}

}  // namespace bookward
