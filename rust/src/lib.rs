#[cxx::bridge(namespace = "bookward")]
pub mod ffi {
    unsafe extern "C++" {
        include!("bookward/bridge.hpp");

        type Log;
        fn open_log_ptr() -> UniquePtr<Log>;
        fn list(self: Pin<&mut Log>, year: i64) -> Result<String>;
    }
}
