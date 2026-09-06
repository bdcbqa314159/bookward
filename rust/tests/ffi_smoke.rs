// The contract-by-construction proof: every GUI verb travels Rust -> cxx ->
// bookward_core -> dataward -> SQLite. Zero database code in Rust.
use bookward_gui::ffi;

#[test]
fn full_verb_set_through_the_cpp_stack() {
    let db = std::env::temp_dir().join("bookward_ffi_smoke.db");
    let _ = std::fs::remove_file(&db);
    std::env::set_var("BOOKWARD_DB", &db);

    let mut log = ffi::open_log_ptr();
    assert!(log.pin_mut().books().unwrap().is_empty());

    // add with explicit year + worked; C++ generates the id
    let msg = log
        .pin_mut()
        .add("Sueños", "Cortázar", "3rd", 2023, 1)
        .unwrap();
    assert!(msg.contains("bk-0001"), "{msg}");

    // rules enforced by the same C++ code as the CLI: empty title throws -> Err
    assert!(log.pin_mut().add("", "", "", 0, -1).is_err());

    // re-read keeps the id; edit fixes text in place
    log.pin_mut().again("bk-0001", 2026).unwrap();
    log.pin_mut()
        .edit("bk-0001", "Sueños de acero", "Cortázar", "3rd", 0)
        .unwrap();

    let rows = log.pin_mut().books().unwrap();
    assert_eq!(rows.len(), 1);
    assert_eq!(rows[0].title, "Sueños de acero");
    assert_eq!(rows[0].years, vec![2023, 2026]);
    assert_eq!(rows[0].worked, 0);

    // year move, single-reading removal guard, full removal
    log.pin_mut().move_year("bk-0001", 2023, 2021).unwrap();
    assert_eq!(log.pin_mut().books().unwrap()[0].years, vec![2021, 2026]);
    log.pin_mut().remove_reading("bk-0001", 2021).unwrap();
    assert!(
        log.pin_mut().remove_reading("bk-0001", 2026).is_err(),
        "last reading must be refused"
    );
    log.pin_mut().remove_book("bk-0001").unwrap();
    assert!(log.pin_mut().books().unwrap().is_empty());
}
