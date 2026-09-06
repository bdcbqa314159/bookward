// Builds the C++ tree (bookward_core + dataward + SOCI + bundled sqlite/fmt)
// via CMake, compiles the cxx bridge against it, and hands cargo the link line.
use std::path::PathBuf;

fn main() {
    let cpp = cmake::Config::new("..")
        .build_target("bookward_core")
        .define("BOOKWARD_BUILD_TESTS", "OFF")
        .build();
    let build = cpp.join("build");

    // The bridge needs bookward's headers plus the FetchContent'd dataward/boost ones.
    let deps = build.join("_deps");
    let includes: Vec<PathBuf> = vec![
        PathBuf::from("../include"),
        deps.join("dataward-src/include"),
        deps.join("boost_describe-src/include"),
        deps.join("boost_mp11-src/include"),
    ];

    let mut bridge = cxx_build::bridge("src/lib.rs");
    bridge.file("../src/bridge.cpp").std("c++20");
    for inc in &includes {
        bridge.include(inc);
    }
    bridge.compile("bookward-bridge");

    // Link the static archives the CMake build produced, wherever they landed.
    for dir in walk_lib_dirs(&build) {
        println!("cargo:rustc-link-search=native={}", dir.display());
    }
    for lib in ["bookward_core", "dataward", "soci_sqlite3", "soci_core"] {
        println!("cargo:rustc-link-lib=static={lib}");
    }
    // SOCI's bundled sqlite3 and fmt archives carry decorated names; find them.
    for archive in find_archives(&build, &["sqlite3", "fmt"]) {
        println!("cargo:rustc-link-lib=static={archive}");
    }
    println!("cargo:rustc-link-lib=c++");

    println!("cargo:rerun-if-changed=../src/bridge.cpp");
    println!("cargo:rerun-if-changed=../include/bookward/bridge.hpp");
    println!("cargo:rerun-if-changed=src/lib.rs");
}

fn walk_lib_dirs(root: &std::path::Path) -> Vec<PathBuf> {
    let mut dirs = vec![];
    let mut stack = vec![root.to_path_buf()];
    while let Some(dir) = stack.pop() {
        if let Ok(entries) = std::fs::read_dir(&dir) {
            let mut has_archive = false;
            for entry in entries.flatten() {
                let path = entry.path();
                if path.is_dir() {
                    stack.push(path);
                } else if path.extension().is_some_and(|e| e == "a") {
                    has_archive = true;
                }
            }
            if has_archive {
                dirs.push(dir);
            }
        }
    }
    dirs
}

fn find_archives(root: &std::path::Path, stems: &[&str]) -> Vec<String> {
    let mut found = vec![];
    let mut stack = vec![root.to_path_buf()];
    while let Some(dir) = stack.pop() {
        if let Ok(entries) = std::fs::read_dir(&dir) {
            for entry in entries.flatten() {
                let path = entry.path();
                if path.is_dir() {
                    stack.push(path);
                } else if let Some(name) = path.file_name().and_then(|n| n.to_str()) {
                    if let Some(libname) =
                        name.strip_prefix("lib").and_then(|n| n.strip_suffix(".a"))
                    {
                        if stems.iter().any(|s| libname.contains(s)) {
                            found.push(libname.to_string());
                        }
                    }
                }
            }
        }
    }
    found.sort();
    found.dedup();
    found
}
