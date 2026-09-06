#[cxx::bridge(namespace = "bookward")]
pub mod ffi {
    /// One book with its reading years — the only data shape crossing the bridge.
    /// worked: -1 = not a technical book, 0 = not worked, 1 = worked.
    #[derive(Clone, Debug)]
    pub struct BookRow {
        pub id: String,
        pub title: String,
        pub author: String,
        pub edition: String,
        pub years: Vec<i64>,
        pub worked: i8,
    }

    unsafe extern "C++" {
        include!("bookward/bridge.hpp");

        type Log;
        fn open_log_ptr() -> UniquePtr<Log>;

        fn books(self: Pin<&mut Log>) -> Result<Vec<BookRow>>;
        fn add(
            self: Pin<&mut Log>,
            title: &str,
            author: &str,
            edition: &str,
            year: i64,
            worked: i8,
        ) -> Result<String>;
        fn again(self: Pin<&mut Log>, id: &str, year: i64) -> Result<String>;
        fn edit(
            self: Pin<&mut Log>,
            id: &str,
            title: &str,
            author: &str,
            edition: &str,
            worked: i8,
        ) -> Result<String>;
        fn move_year(self: Pin<&mut Log>, id: &str, from: i64, to: i64) -> Result<String>;
        fn remove_book(self: Pin<&mut Log>, id: &str) -> Result<String>;
        fn remove_reading(self: Pin<&mut Log>, id: &str, year: i64) -> Result<String>;
        fn report(self: Pin<&mut Log>, year: i64, out_dir: &str) -> Result<String>;
    }
}
