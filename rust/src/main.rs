use bookward_gui::ffi::{self, BookRow};
use eframe::egui;

fn worked_label(worked: i8) -> &'static str {
    match worked {
        1 => "worked",
        0 => "not worked",
        _ => "—",
    }
}

fn years_str(years: &[i64]) -> String {
    years
        .iter()
        .map(|y| y.to_string())
        .collect::<Vec<_>>()
        .join(", ")
}

fn parse_year(s: &str) -> Result<i64, String> {
    if s.trim().is_empty() {
        return Ok(0); // bridge convention: 0 = default/current
    }
    s.trim().parse().map_err(|_| format!("not a year: {s}"))
}

#[derive(Default)]
struct Form {
    title: String,
    author: String,
    edition: String,
    year: String,
    worked: i8,
}

struct App {
    log: cxx::UniquePtr<ffi::Log>,
    rows: Vec<BookRow>,
    filter: String,
    selected: Option<String>,
    status: String,
    show_add: bool,
    add: Form,
    edit: Form,
    again_year: String,
    move_from: String,
    move_to: String,
    confirm_remove: bool,
}

impl App {
    fn new() -> Self {
        let mut app = App {
            log: ffi::open_log_ptr(),
            rows: vec![],
            filter: String::new(),
            selected: None,
            status: "ready".into(),
            show_add: false,
            add: Form::default(),
            edit: Form::default(),
            again_year: String::new(),
            move_from: String::new(),
            move_to: String::new(),
            confirm_remove: false,
        };
        app.refresh();
        app
    }

    fn refresh(&mut self) {
        match self.log.pin_mut().books() {
            Ok(rows) => self.rows = rows,
            Err(e) => self.status = format!("error: {e}"),
        }
        if let Some(id) = &self.selected {
            if !self.rows.iter().any(|r| &r.id == id) {
                self.selected = None;
            }
        }
    }

    fn done(&mut self, result: Result<String, cxx::Exception>) {
        self.status = match result {
            Ok(msg) => msg,
            Err(e) => format!("error: {e}"),
        };
        self.refresh();
    }

    fn selected_row(&self) -> Option<BookRow> {
        let id = self.selected.as_ref()?;
        self.rows.iter().find(|r| &r.id == id).cloned()
    }

    fn select(&mut self, row: &BookRow) {
        self.selected = Some(row.id.clone());
        self.edit = Form {
            title: row.title.clone(),
            author: row.author.clone(),
            edition: row.edition.clone(),
            year: String::new(),
            worked: row.worked,
        };
        self.again_year.clear();
        self.move_from.clear();
        self.move_to.clear();
        self.confirm_remove = false;
    }

    fn worked_combo(ui: &mut egui::Ui, id: &str, value: &mut i8) {
        egui::ComboBox::from_id_salt(id)
            .selected_text(match value {
                1 => "worked",
                0 => "not worked",
                _ => "not a technical book",
            })
            .show_ui(ui, |ui| {
                ui.selectable_value(value, -1, "not a technical book");
                ui.selectable_value(value, 1, "worked");
                ui.selectable_value(value, 0, "not worked");
            });
    }

    fn side_panel(&mut self, ui: &mut egui::Ui) {
        let Some(row) = self.selected_row() else {
            ui.label("select a book");
            return;
        };
        ui.heading(&row.id);
        ui.label(format!("pdf: {}.pdf", row.id));
        ui.label(format!("read in {}", years_str(&row.years)));
        ui.separator();

        ui.label("title");
        ui.text_edit_singleline(&mut self.edit.title);
        ui.label("author");
        ui.text_edit_singleline(&mut self.edit.author);
        ui.label("edition");
        ui.text_edit_singleline(&mut self.edit.edition);
        Self::worked_combo(ui, "edit-worked", &mut self.edit.worked);
        if ui.button("save").clicked() {
            let r = self.log.pin_mut().edit(
                &row.id,
                &self.edit.title.clone(),
                &self.edit.author.clone(),
                &self.edit.edition.clone(),
                self.edit.worked,
            );
            self.done(r);
        }
        ui.separator();

        ui.horizontal(|ui| {
            ui.label("re-read year");
            ui.add(egui::TextEdit::singleline(&mut self.again_year).desired_width(60.0));
        });
        if ui.button("log re-read").clicked() {
            match parse_year(&self.again_year) {
                Ok(y) => {
                    let r = self.log.pin_mut().again(&row.id, y);
                    self.done(r);
                }
                Err(e) => self.status = e,
            }
        }
        ui.separator();

        ui.horizontal(|ui| {
            ui.label("move year");
            ui.add(egui::TextEdit::singleline(&mut self.move_from).desired_width(60.0));
            ui.label("→");
            ui.add(egui::TextEdit::singleline(&mut self.move_to).desired_width(60.0));
        });
        if ui.button("move").clicked() {
            match (parse_year(&self.move_from), parse_year(&self.move_to)) {
                (Ok(from), Ok(to)) if to != 0 => {
                    let r = self.log.pin_mut().move_year(&row.id, from, to);
                    self.done(r);
                }
                _ => self.status = "move needs a target year".into(),
            }
        }
        ui.separator();

        if row.years.len() > 1 {
            ui.label("forget one reading:");
            ui.horizontal_wrapped(|ui| {
                for y in row.years.clone() {
                    if ui.button(y.to_string()).clicked() {
                        let r = self.log.pin_mut().remove_reading(&row.id, y);
                        self.done(r);
                    }
                }
            });
        }
        ui.checkbox(&mut self.confirm_remove, "yes, remove this book entirely");
        if ui
            .add_enabled(self.confirm_remove, egui::Button::new("remove book"))
            .clicked()
        {
            let r = self.log.pin_mut().remove_book(&row.id);
            self.done(r);
        }
    }

    fn table(&mut self, ui: &mut egui::Ui) {
        let needle = self.filter.to_lowercase();
        let rows: Vec<BookRow> = self
            .rows
            .iter()
            .filter(|r| {
                needle.is_empty()
                    || r.title.to_lowercase().contains(&needle)
                    || r.author.to_lowercase().contains(&needle)
            })
            .cloned()
            .collect();

        egui::ScrollArea::vertical().show(ui, |ui| {
            egui::Grid::new("books")
                .striped(true)
                .num_columns(6)
                .show(ui, |ui| {
                    for header in ["id", "title", "author", "edition", "years", "worked"] {
                        ui.strong(header);
                    }
                    ui.end_row();
                    for row in &rows {
                        let selected = self.selected.as_deref() == Some(row.id.as_str());
                        if ui.selectable_label(selected, &row.id).clicked() {
                            self.select(row);
                        }
                        ui.label(&row.title);
                        ui.label(&row.author);
                        ui.label(&row.edition);
                        ui.label(years_str(&row.years));
                        ui.label(worked_label(row.worked));
                        ui.end_row();
                    }
                });
        });
    }

    fn add_form(&mut self, ui: &mut egui::Ui) {
        ui.horizontal(|ui| {
            ui.label("title");
            ui.text_edit_singleline(&mut self.add.title);
            ui.label("author");
            ui.text_edit_singleline(&mut self.add.author);
            ui.label("edition");
            ui.add(egui::TextEdit::singleline(&mut self.add.edition).desired_width(80.0));
            ui.label("year");
            ui.add(egui::TextEdit::singleline(&mut self.add.year).desired_width(60.0));
            Self::worked_combo(ui, "add-worked", &mut self.add.worked);
            if ui.button("add").clicked() {
                match parse_year(&self.add.year) {
                    Ok(y) => {
                        let r = self.log.pin_mut().add(
                            &self.add.title.clone(),
                            &self.add.author.clone(),
                            &self.add.edition.clone(),
                            y,
                            self.add.worked,
                        );
                        if r.is_ok() {
                            self.add = Form::default();
                            self.show_add = false;
                        }
                        self.done(r);
                    }
                    Err(e) => self.status = e,
                }
            }
        });
    }

    fn stats_line(&self) -> String {
        use std::collections::BTreeMap;
        let mut per_year: BTreeMap<i64, i64> = BTreeMap::new();
        let mut readings = 0;
        for row in &self.rows {
            for y in &row.years {
                *per_year.entry(*y).or_default() += 1;
                readings += 1;
            }
        }
        let years = per_year
            .iter()
            .rev()
            .map(|(y, n)| format!("{y}: {n}"))
            .collect::<Vec<_>>()
            .join("   ");
        format!(
            "{} books, {} readings   |   {}",
            self.rows.len(),
            readings,
            years
        )
    }
}

impl eframe::App for App {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        egui::TopBottomPanel::top("top").show(ctx, |ui| {
            ui.horizontal(|ui| {
                ui.heading("bookward");
                ui.separator();
                ui.label("find");
                ui.add(egui::TextEdit::singleline(&mut self.filter).desired_width(200.0));
                if ui
                    .button(if self.show_add { "close" } else { "add book" })
                    .clicked()
                {
                    self.show_add = !self.show_add;
                }
                if ui.button("report").clicked() {
                    let r = self.log.pin_mut().report(0, "");
                    if let Ok(msg) = &r {
                        if let Some(path) = msg.strip_prefix("wrote ") {
                            if let Some(pdf) = path.split(" and ").next() {
                                let _ = std::process::Command::new("open").arg(pdf).spawn();
                            }
                        }
                    }
                    self.done(r);
                }
                if ui.button("refresh").clicked() {
                    self.refresh();
                }
            });
            if self.show_add {
                self.add_form(ui);
            }
        });
        egui::TopBottomPanel::bottom("status").show(ctx, |ui| {
            ui.label(self.stats_line());
            ui.label(egui::RichText::new(&self.status).weak());
        });
        egui::SidePanel::right("detail")
            .default_width(280.0)
            .show(ctx, |ui| self.side_panel(ui));
        egui::CentralPanel::default().show(ctx, |ui| self.table(ui));
    }
}

fn main() -> eframe::Result {
    eframe::run_native(
        "bookward",
        eframe::NativeOptions::default(),
        Box::new(|_| Ok(Box::new(App::new()))),
    )
}
