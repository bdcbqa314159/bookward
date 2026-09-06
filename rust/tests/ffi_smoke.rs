// The contract-by-construction proof: Rust calls through cxx into
// bookward_core -> dataward -> SOCI -> SQLite, no Rust DB code anywhere.
use bookward_gui::ffi;

#[test]
fn opens_and_lists_through_the_cpp_stack() {
    let db = std::env::temp_dir().join("bookward_ffi_smoke.db");
    let _ = std::fs::remove_file(&db);
    std::env::set_var("BOOKWARD_DB", &db);

    let mut log = ffi::open_log_ptr();
    let listing = log.pin_mut().list(0).expect("list should not fail");
    assert_eq!(listing, "(no books)");
}
