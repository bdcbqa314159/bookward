#pragma once

#include <optional>
#include <string>

#include "bookward/book.hpp"

namespace bookward {

// Each command returns the line to print; invalid input throws
// std::runtime_error with the message the CLI shows (exit 1).

std::string cmd_add(dataward::Store& store, const std::string& id, const std::string& title,
                    const std::string& author, std::int64_t pages);
std::string cmd_progress(dataward::Store& store, const std::string& id, std::int64_t page);
std::string cmd_finish(dataward::Store& store, const std::string& id,
                       std::optional<std::int64_t> rating);
std::string cmd_shelve(dataward::Store& store, const std::string& id);
// status_filter: "" = all, else reading|finished|shelved.
std::string cmd_list(dataward::Store& store, const std::string& status_filter);
// Per-year LaTeX report (books finished in `year` + in-progress bars) written
// to out_dir ("" = cwd); compile=true also runs latexmk for the PDF.
std::string cmd_report(dataward::Store& store, std::int64_t year, const std::string& out_dir,
                       bool compile);

}  // namespace bookward
