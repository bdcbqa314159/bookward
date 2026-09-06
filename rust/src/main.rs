use bookward_gui::ffi;
use eframe::egui;

struct App {
    log: cxx::UniquePtr<ffi::Log>,
    listing: String,
}

impl App {
    fn refresh(&mut self) {
        self.listing = match self.log.pin_mut().list(0) {
            Ok(text) => text,
            Err(e) => format!("error: {e}"),
        };
    }
}

impl eframe::App for App {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("bookward");
            if ui.button("refresh").clicked() {
                self.refresh();
            }
            ui.separator();
            egui::ScrollArea::vertical().show(ui, |ui| {
                ui.monospace(&self.listing);
            });
        });
    }
}

fn main() -> eframe::Result {
    let mut app = App {
        log: ffi::open_log_ptr(),
        listing: String::new(),
    };
    app.refresh();
    eframe::run_native(
        "bookward",
        eframe::NativeOptions::default(),
        Box::new(|_| Ok(Box::new(app))),
    )
}
