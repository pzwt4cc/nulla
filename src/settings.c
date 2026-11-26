#include "nulla.h"

#include <string.h>

typedef struct {
	NullaWindow *win;
	GtkWidget *lang_dropdown;
	GPtrArray *lang_ids;
} NullaSettingsUI;

typedef struct {
	const char *id;
	const char *name;
} NullaLangItem;

static char *nulla_config_path(void)
{
	return g_build_filename(g_get_user_config_dir(), "nulla", "settings.ini", NULL);
}

void nulla_config_init(NullaConfig *cfg)
{
	cfg->show_line_numbers = TRUE;
	cfg->theme_mode = NULLA_THEME_GTK;
	cfg->syntax_enabled = TRUE;
	cfg->language_id = g_strdup("auto");
	cfg->zoom_percent = 100;
}

void nulla_config_free(NullaConfig *cfg)
{
	g_clear_pointer(&cfg->language_id, g_free);
}

void nulla_config_load(NullaConfig *cfg)
{
	char *path = nulla_config_path();
	GKeyFile *kf = g_key_file_new();

	if (g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL)) {
		if (g_key_file_has_key(kf, "editor", "line_numbers", NULL)) {
			cfg->show_line_numbers = g_key_file_get_boolean(kf, "editor", "line_numbers", NULL);
		}
		if (g_key_file_has_key(kf, "editor", "theme", NULL)) {
			int mode = g_key_file_get_integer(kf, "editor", "theme", NULL);
			if (mode >= NULLA_THEME_GTK && mode <= NULLA_THEME_DARK) {
				cfg->theme_mode = (NullaThemeMode)mode;
			}
		}
		if (g_key_file_has_key(kf, "editor", "syntax", NULL)) {
			cfg->syntax_enabled = g_key_file_get_boolean(kf, "editor", "syntax", NULL);
		}
		if (g_key_file_has_key(kf, "editor", "language", NULL)) {
			char *lang = g_key_file_get_string(kf, "editor", "language", NULL);
			if (lang != NULL && lang[0] != '\0') {
				g_free(cfg->language_id);
				cfg->language_id = lang;
			} else {
				g_free(lang);
			}
		}
		if (g_key_file_has_key(kf, "editor", "zoom", NULL)) {
			int zoom = g_key_file_get_integer(kf, "editor", "zoom", NULL);
			if (zoom >= 50 && zoom <= 300) {
				cfg->zoom_percent = zoom;
			}
		}
	}

	g_key_file_unref(kf);
	g_free(path);
}

void nulla_config_save(const NullaConfig *cfg)
{
	char *path = nulla_config_path();
	char *dir = g_path_get_dirname(path);
	GKeyFile *kf = g_key_file_new();
	gsize len = 0;
	char *data = NULL;

	g_mkdir_with_parents(dir, 0755);

	g_key_file_set_boolean(kf, "editor", "line_numbers", cfg->show_line_numbers);
	g_key_file_set_integer(kf, "editor", "theme", (int)cfg->theme_mode);
	g_key_file_set_boolean(kf, "editor", "syntax", cfg->syntax_enabled);
	g_key_file_set_string(kf, "editor", "language", cfg->language_id ? cfg->language_id : "auto");
	g_key_file_set_integer(kf, "editor", "zoom", cfg->zoom_percent);

	data = g_key_file_to_data(kf, &len, NULL);
	if (data != NULL) {
		g_file_set_contents(path, data, (gssize)len, NULL);
		g_free(data);
	}

	g_key_file_unref(kf);
	g_free(dir);
	g_free(path);
}

static void nulla_settings_ui_free(gpointer data)
{
	NullaSettingsUI *ui = data;
	if (ui == NULL) {
		return;
	}
	if (ui->lang_ids != NULL) {
		g_ptr_array_unref(ui->lang_ids);
	}
	g_free(ui);
}

static void on_settings_destroy(GtkWidget *widget, gpointer user_data)
{
	(void)widget;
	NullaWindow *win = user_data;
	win->settings_window = NULL;
}

static void on_line_numbers_changed(GObject *obj, GParamSpec *pspec, gpointer user_data)
{
	(void)pspec;
	NullaSettingsUI *ui = user_data;
	ui->win->config->show_line_numbers = gtk_switch_get_active(GTK_SWITCH(obj));
	nulla_window_apply_settings(ui->win);
	nulla_config_save(ui->win->config);
}

static void on_theme_changed(GObject *obj, GParamSpec *pspec, gpointer user_data)
{
	(void)pspec;
	NullaSettingsUI *ui = user_data;
	guint selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(obj));
	if (selected <= NULLA_THEME_DARK) {
		ui->win->config->theme_mode = (NullaThemeMode)selected;
		nulla_window_apply_theme(ui->win);
		nulla_config_save(ui->win->config);
	}
}

static void on_syntax_changed(GObject *obj, GParamSpec *pspec, gpointer user_data)
{
	(void)pspec;
	NullaSettingsUI *ui = user_data;
	gboolean active = gtk_switch_get_active(GTK_SWITCH(obj));
	ui->win->config->syntax_enabled = active;
	gtk_widget_set_sensitive(ui->lang_dropdown, active);
	nulla_window_apply_language(ui->win);
	nulla_config_save(ui->win->config);
}

static void on_language_changed(GObject *obj, GParamSpec *pspec, gpointer user_data)
{
	(void)pspec;
	NullaSettingsUI *ui = user_data;
	guint idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(obj));
	if (idx < ui->lang_ids->len) {
		const char *id = g_ptr_array_index(ui->lang_ids, idx);
		g_free(ui->win->config->language_id);
		ui->win->config->language_id = g_strdup(id);
		nulla_window_apply_language(ui->win);
		nulla_config_save(ui->win->config);
	}
}

static void on_zoom_changed(GtkSpinButton *spin, gpointer user_data)
{
	NullaSettingsUI *ui = user_data;
	ui->win->config->zoom_percent = gtk_spin_button_get_value_as_int(spin);
	nulla_window_apply_zoom(ui->win);
	nulla_config_save(ui->win->config);
}

static int compare_lang_items(gconstpointer a, gconstpointer b)
{
	const NullaLangItem *la = a;
	const NullaLangItem *lb = b;
	return g_ascii_strcasecmp(la->name, lb->name);
}

void nulla_settings_show(NullaWindow *win)
{
	if (win->settings_window != NULL) {
		gtk_window_present(GTK_WINDOW(win->settings_window));
		return;
	}

	GtkWidget *sw = gtk_window_new();
	gtk_widget_add_css_class(sw, "nulla-settings-window");
	gtk_window_set_title(GTK_WINDOW(sw), "Settings");
	gtk_window_set_transient_for(GTK_WINDOW(sw), GTK_WINDOW(win->window));
	gtk_window_set_modal(GTK_WINDOW(sw), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(sw), FALSE);

	NullaSettingsUI *ui = g_new0(NullaSettingsUI, 1);
	ui->win = win;
	ui->lang_ids = g_ptr_array_new_with_free_func(g_free);

	g_object_set_data_full(G_OBJECT(sw), "nulla-settings-ui", ui, nulla_settings_ui_free);
	g_signal_connect(sw, "destroy", G_CALLBACK(on_settings_destroy), win);

	GtkWidget *grid = gtk_grid_new();
	gtk_widget_add_css_class(grid, "nulla-settings-card");
	gtk_grid_set_row_spacing(GTK_GRID(grid), 14);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 24);
	gtk_widget_set_margin_top(grid, 20);
	gtk_widget_set_margin_bottom(grid, 20);
	gtk_widget_set_margin_start(grid, 20);
	gtk_widget_set_margin_end(grid, 20);
	gtk_window_set_child(GTK_WINDOW(sw), grid);

	GtkWidget *lbl_lines = gtk_label_new("Line numbers");
	gtk_widget_set_halign(lbl_lines, GTK_ALIGN_START);
	GtkWidget *switch_lines = gtk_switch_new();
	gtk_widget_set_halign(switch_lines, GTK_ALIGN_END);
	gtk_switch_set_active(GTK_SWITCH(switch_lines), win->config->show_line_numbers);
	g_signal_connect(switch_lines, "notify::active", G_CALLBACK(on_line_numbers_changed), ui);
	gtk_grid_attach(GTK_GRID(grid), lbl_lines, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), switch_lines, 1, 0, 1, 1);

	GtkWidget *lbl_theme = gtk_label_new("Theme");
	gtk_widget_set_halign(lbl_theme, GTK_ALIGN_START);
	const char * const themes[] = {"GTK", "Light", "Dark", NULL};
	GtkWidget *dd_theme = gtk_drop_down_new_from_strings(themes);
	gtk_drop_down_set_selected(GTK_DROP_DOWN(dd_theme), (guint)win->config->theme_mode);
	g_signal_connect(dd_theme, "notify::selected", G_CALLBACK(on_theme_changed), ui);
	gtk_grid_attach(GTK_GRID(grid), lbl_theme, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), dd_theme, 1, 1, 1, 1);

	GtkWidget *lbl_syntax = gtk_label_new("Syntax highlighting");
	gtk_widget_set_halign(lbl_syntax, GTK_ALIGN_START);
	GtkWidget *switch_syntax = gtk_switch_new();
	gtk_widget_set_halign(switch_syntax, GTK_ALIGN_END);
	gtk_switch_set_active(GTK_SWITCH(switch_syntax), win->config->syntax_enabled);
	gtk_grid_attach(GTK_GRID(grid), lbl_syntax, 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), switch_syntax, 1, 2, 1, 1);

	GtkSourceLanguageManager *lm = gtk_source_language_manager_get_default();
	const char * const *raw_ids = gtk_source_language_manager_get_language_ids(lm);
	GArray *items = g_array_new(FALSE, FALSE, sizeof(NullaLangItem));

	if (raw_ids != NULL) {
		for (int i = 0; raw_ids[i] != NULL; i++) {
			GtkSourceLanguage *lang = gtk_source_language_manager_get_language(lm, raw_ids[i]);
			if (lang != NULL && !gtk_source_language_get_hidden(lang)) {
				NullaLangItem item;
				item.id = gtk_source_language_get_id(lang);
				item.name = gtk_source_language_get_name(lang);
				g_array_append_val(items, item);
			}
		}
	}
	g_array_sort(items, compare_lang_items);

	GPtrArray *lang_names = g_ptr_array_new();
	g_ptr_array_add(lang_names, (gpointer)"Auto");
	g_ptr_array_add(ui->lang_ids, g_strdup("auto"));

	guint selected_lang = 0;
	for (guint i = 0; i < items->len; i++) {
		NullaLangItem *item = &g_array_index(items, NullaLangItem, i);
		g_ptr_array_add(lang_names, (gpointer)item->name);
		g_ptr_array_add(ui->lang_ids, g_strdup(item->id));
		if (win->config->language_id != NULL &&
		    g_strcmp0(win->config->language_id, item->id) == 0) {
			selected_lang = i + 1;
		}
	}
	g_ptr_array_add(lang_names, NULL);

	GtkWidget *lbl_lang = gtk_label_new("Language");
	gtk_widget_set_halign(lbl_lang, GTK_ALIGN_START);
	GtkWidget *dd_lang = gtk_drop_down_new_from_strings((const char * const *)lang_names->pdata);
	gtk_drop_down_set_enable_search(GTK_DROP_DOWN(dd_lang), TRUE);
	gtk_drop_down_set_selected(GTK_DROP_DOWN(dd_lang), selected_lang);
	gtk_widget_set_sensitive(dd_lang, win->config->syntax_enabled);
	ui->lang_dropdown = dd_lang;

	g_ptr_array_free(lang_names, TRUE);
	g_array_free(items, TRUE);

	g_signal_connect(dd_lang, "notify::selected", G_CALLBACK(on_language_changed), ui);
	g_signal_connect(switch_syntax, "notify::active", G_CALLBACK(on_syntax_changed), ui);
	gtk_grid_attach(GTK_GRID(grid), lbl_lang, 0, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), dd_lang, 1, 3, 1, 1);

	GtkWidget *lbl_zoom = gtk_label_new("Zoom (%)");
	gtk_widget_set_halign(lbl_zoom, GTK_ALIGN_START);
	GtkWidget *spin_zoom = gtk_spin_button_new_with_range(50.0, 300.0, 10.0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_zoom), (double)win->config->zoom_percent);
	g_signal_connect(spin_zoom, "value-changed", G_CALLBACK(on_zoom_changed), ui);
	gtk_grid_attach(GTK_GRID(grid), lbl_zoom, 0, 4, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_zoom, 1, 4, 1, 1);

	win->settings_window = sw;
	gtk_window_present(GTK_WINDOW(sw));
}
