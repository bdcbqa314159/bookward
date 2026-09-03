#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "bookward/commands.hpp"

namespace bookward {

namespace {

// LaTeX-escape user text (titles, authors, notes).
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

std::string iso(const std::chrono::year_month_day& d) {
  char buf[16];
  std::snprintf(buf, sizeof buf, "%04d-%02u-%02u", static_cast<int>(d.year()),
                static_cast<unsigned>(d.month()), static_cast<unsigned>(d.day()));
  return buf;
}

constexpr std::array<const char*, 12> kMonths = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

}  // namespace

std::string cmd_report(dataward::Store& store, std::int64_t year, const std::string& out_dir,
                       bool compile) {
  const std::string y = std::to_string(year);
  auto finished =
      store.where<Book>("\"finished\" >= :b0 AND \"finished\" <= :b1 ORDER BY \"finished\"",
                        y + "-01-01", y + "-12-31");
  auto reading = store.where<Book>("\"status\" = 'reading' ORDER BY \"id\"");

  std::int64_t total_pages = 0;
  std::array<std::int64_t, 12> pages_by_month{};
  std::int64_t rating_sum = 0, rated = 0;
  for (const auto& b : finished) {
    total_pages += b.pages;
    // Convention: a book's pages count toward its finish month.
    pages_by_month[static_cast<unsigned>(b.finished->month()) - 1] += b.pages;
    if (b.rating) {
      rating_sum += *b.rating;
      ++rated;
    }
  }

  std::string doc;
  doc += "\\documentclass[11pt,a4paper]{article}\n";
  doc += "\\usepackage[a4paper,margin=1in]{geometry}\n";
  doc += "\\usepackage{microtype}\n\\usepackage{booktabs}\n\\usepackage{tikz}\n";
  doc += "\\usepackage[hidelinks]{hyperref}\n";
  doc += "\\title{Reading Report " + y + "}\n\\author{bookward}\n\\date{\\today}\n";
  doc += "\\begin{document}\n\\maketitle\n";

  doc += "\\section*{Summary}\n";
  doc += std::to_string(finished.size()) + " book" + (finished.size() == 1 ? "" : "s") +
         " finished, " + std::to_string(total_pages) + "~pages total";
  if (rated > 0) {
    char avg[16];
    std::snprintf(avg, sizeof avg, "%.1f", static_cast<double>(rating_sum) / rated);
    doc += ", average rating " + std::string(avg) + "/5";
  }
  doc += ".\n";

  if (!finished.empty()) {
    doc += "\\section*{Finished}\n\\begin{tabular}{llrlr}\n\\toprule\n";
    doc += "Title & Author & Pages & Finished & Rating \\\\\n\\midrule\n";
    for (const auto& b : finished) {
      doc += tex(b.title) + " & " + tex(b.author) + " & " + std::to_string(b.pages) + " & " +
             iso(*b.finished) + " & " + (b.rating ? std::to_string(*b.rating) + "/5" : "---") +
             " \\\\\n";
    }
    doc += "\\bottomrule\n\\end{tabular}\n";

    doc += "\\section*{Pages per Month}\n\\begin{tabular}{lr}\n\\toprule\n";
    doc += "Month & Pages \\\\\n\\midrule\n";
    for (int m = 0; m < 12; ++m)
      if (pages_by_month[static_cast<std::size_t>(m)] > 0)
        doc += std::string(kMonths[static_cast<std::size_t>(m)]) + " & " +
               std::to_string(pages_by_month[static_cast<std::size_t>(m)]) + " \\\\\n";
    doc += "\\bottomrule\n\\end{tabular}\n";
  }

  if (!reading.empty()) {
    doc += "\\section*{In Progress}\n";
    for (const auto& b : reading) {
      const double frac =
          b.pages > 0 ? static_cast<double>(b.current_page) / static_cast<double>(b.pages) : 0.0;
      char fracbuf[16];
      std::snprintf(fracbuf, sizeof fracbuf, "%.3f", frac);
      char pct[16];
      std::snprintf(pct, sizeof pct, "%.0f", frac * 100.0);
      doc += tex(b.title) + " --- page " + std::to_string(b.current_page) + "/" +
             std::to_string(b.pages) + "\\\\[2pt]\n";
      doc += "\\begin{tikzpicture}\n";
      doc += "\\draw[fill=black!10] (0,0) rectangle (10,0.28);\n";
      doc += "\\draw[fill=black!60] (0,0) rectangle (" + std::string(fracbuf) + "*10,0.28);\n";
      doc += "\\node[right] at (10.1,0.14) {\\footnotesize " + std::string(pct) + "\\%};\n";
      doc += "\\end{tikzpicture}\\\\[6pt]\n";
    }
  }

  doc += "\\end{document}\n";

  const auto dir = std::filesystem::path(out_dir.empty() ? "." : out_dir);
  std::filesystem::create_directories(dir);
  const auto tex_path = dir / ("reading-report-" + y + ".tex");
  std::ofstream(tex_path) << doc;

  if (!compile) return "wrote " + tex_path.string();

  const std::string cmd = "latexmk -pdf -interaction=nonstopmode -output-directory=\"" +
                          dir.string() + "\" \"" + tex_path.string() + "\" > /dev/null 2>&1";
  if (std::system(cmd.c_str()) != 0)
    return "wrote " + tex_path.string() + " (latexmk failed or missing — compile it manually)";
  return "wrote " + (dir / ("reading-report-" + y + ".pdf")).string();
}

}  // namespace bookward
