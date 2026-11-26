#include "nulla.h"

#include <string.h>

static gboolean nulla_gtk_is_dark(void)
{
	GtkSettings *gs = gtk_settings_get_default();
	if (gs == NULL) {
		return FALSE;
	}

#if GTK_CHECK_VERSION(4, 20, 0)
	GtkInterfaceColorScheme scheme = GTK_INTERFACE_COLOR_SCHEME_DEFAULT;
	g_object_get(gs, "gtk-interface-color-scheme", &scheme, NULL);
	if (scheme == GTK_INTERFACE_COLOR_SCHEME_DARK) {
		return TRUE;
	}
	if (scheme == GTK_INTERFACE_COLOR_SCHEME_LIGHT) {
		return FALSE;
	}
#else
	gboolean prefer_dark = FALSE;
	g_object_get(gs, "gtk-application-prefer-dark-theme", &prefer_dark, NULL);
	if (prefer_dark) {
		return TRUE;
	}
#endif

	char *theme_name = NULL;
	g_object_get(gs, "gtk-theme-name", &theme_name, NULL);
	if (theme_name != NULL) {
		char *lower = g_ascii_strdown(theme_name, -1);
		gboolean has_dark = (strstr(lower, "dark") != NULL);
		g_free(lower);
		g_free(theme_name);
		if (has_dark) {
			return TRUE;
		}
	}

	return FALSE;
}

static void nulla_window_sync_theme_classes(NullaWindow *win, gboolean dark)
{
	GtkWidget *targets[2] = { win->window, win->settings_window };

	for (int i = 0; i < 2; i++) {
		if (targets[i] == NULL) {
			continue;
		}
		gtk_widget_remove_css_class(targets[i], "nulla-light");
		gtk_widget_remove_css_class(targets[i], "nulla-dark");
		gtk_widget_remove_css_class(targets[i], "nulla-gtk");

		if (win->config->theme_mode == NULLA_THEME_LIGHT) {
			gtk_widget_add_css_class(targets[i], "nulla-light");
		} else if (win->config->theme_mode == NULLA_THEME_DARK) {
			gtk_widget_add_css_class(targets[i], "nulla-dark");
		} else {
			gtk_widget_add_css_class(targets[i], "nulla-gtk");
			gtk_widget_add_css_class(targets[i], dark ? "nulla-dark" : "nulla-light");
		}
	}
}

static void nulla_window_update_source_scheme(NullaWindow *win)
{
	gboolean dark = FALSE;

	if (win->config->theme_mode == NULLA_THEME_DARK) {
		dark = TRUE;
	} else if (win->config->theme_mode == NULLA_THEME_LIGHT) {
		dark = FALSE;
	} else {
		dark = nulla_gtk_is_dark();
	}

	nulla_window_sync_theme_classes(win, dark);

	GtkSourceStyleSchemeManager *sm = gtk_source_style_scheme_manager_get_default();
	GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(
		sm, dark ? "Adwaita-dark" : "Adwaita");
	if (scheme == NULL) {
		scheme = gtk_source_style_scheme_manager_get_scheme(
			sm, dark ? "oblivion" : "classic");
	}
	gtk_source_buffer_set_style_scheme(win->buffer, scheme);
}

void nulla_window_apply_theme(NullaWindow *win)
{
	GtkSettings *gs = gtk_settings_get_default();

	if (gs != NULL) {
		if (win->config->theme_mode == NULLA_THEME_LIGHT) {
#if GTK_CHECK_VERSION(4, 20, 0)
			g_object_set(gs, "gtk-interface-color-scheme",
			             GTK_INTERFACE_COLOR_SCHEME_LIGHT, NULL);
#else
			g_object_set(gs, "gtk-application-prefer-dark-theme", FALSE, NULL);
#endif
		} else if (win->config->theme_mode == NULLA_THEME_DARK) {
#if GTK_CHECK_VERSION(4, 20, 0)
			g_object_set(gs, "gtk-interface-color-scheme",
			             GTK_INTERFACE_COLOR_SCHEME_DARK, NULL);
#else
			g_object_set(gs, "gtk-application-prefer-dark-theme", TRUE, NULL);
#endif
		} else {
#if GTK_CHECK_VERSION(4, 20, 0)
			gtk_settings_reset_property(gs, "gtk-interface-color-scheme");
#else
			gtk_settings_reset_property(gs, "gtk-application-prefer-dark-theme");
#endif
		}
	}

	nulla_window_update_source_scheme(win);
}

void nulla_window_update_status(NullaWindow *win)
{
	if (win->status_pos_label != NULL) {
		GtkTextIter iter;
		GtkTextMark *insert = gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(win->buffer));
		gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(win->buffer), &iter, insert);
		int line = gtk_text_iter_get_line(&iter) + 1;
		int col = gtk_text_iter_get_line_offset(&iter) + 1;
		char *pos = g_strdup_printf("Ln %d, Col %d", line, col);
		gtk_label_set_text(GTK_LABEL(win->status_pos_label), pos);
		g_free(pos);
	}

	if (win->status_lang_label != NULL) {
		if (!win->config->syntax_enabled) {
			gtk_label_set_text(GTK_LABEL(win->status_lang_label), "Plain");
		} else {
			GtkSourceLanguage *lang = gtk_source_buffer_get_language(win->buffer);
			const char *name = lang != NULL ? gtk_source_language_get_name(lang) : "Plain";
			gtk_label_set_text(GTK_LABEL(win->status_lang_label), name);
		}
	}

	if (win->status_zoom_btn != NULL) {
		char *zoom = g_strdup_printf("%d%%", win->config->zoom_percent);
		gtk_button_set_label(GTK_BUTTON(win->status_zoom_btn), zoom);
		g_free(zoom);
	}
}

void nulla_window_apply_language(NullaWindow *win)
{
	if (!win->config->syntax_enabled) {
		gtk_source_buffer_set_highlight_syntax(win->buffer, FALSE);
		gtk_source_buffer_set_language(win->buffer, NULL);
		nulla_window_update_status(win);
		return;
	}

	GtkSourceLanguageManager *lm = gtk_source_language_manager_get_default();
	GtkSourceLanguage *lang = NULL;

	if (win->config->language_id != NULL &&
	    g_strcmp0(win->config->language_id, "auto") != 0) {
		lang = gtk_source_language_manager_get_language(lm, win->config->language_id);
	} else {
		GFile *loc = gtk_source_file_get_location(win->source_file);
		if (loc != NULL) {
			char *basename = g_file_get_basename(loc);
			lang = gtk_source_language_manager_guess_language(lm, basename, NULL);
			g_free(basename);
		}
	}

	gtk_source_buffer_set_language(win->buffer, lang);
	gtk_source_buffer_set_highlight_syntax(win->buffer, TRUE);
	nulla_window_update_status(win);
}

void nulla_window_apply_zoom(NullaWindow *win)
{
	double pt = 11.0 * ((double)win->config->zoom_percent / 100.0);
	char buf[G_ASCII_DTOSTR_BUF_SIZE];
	g_ascii_formatd(buf, sizeof(buf), "%.2f", pt);

	static const char * const css_base =
		"window.nulla-window { border-radius: 18px; }\n"
		"window.nulla-window.nulla-light { background-color: #f3f4f7; color: #1a1c20; }\n"
		"window.nulla-window.nulla-dark { background-color: #121316; color: #e4e6eb; }\n"
		"window.nulla-settings-window { border-radius: 22px; }\n"
		"window.nulla-settings-window.nulla-light { background-color: #f3f4f7; color: #1a1c20; }\n"
		"window.nulla-settings-window.nulla-dark { background-color: #121316; color: #e4e6eb; }\n"
		".nulla-topbar { padding: 8px 10px 6px 10px; min-height: 44px; background: transparent; }\n"
		".nulla-pill-group { background-color: alpha(currentColor, 0.06); border-radius: 9999px; padding: 3px; }\n"
		"button.nulla-pill-btn { border-radius: 9999px; min-height: 32px; min-width: 32px; padding: 4px 12px; border: none; background-color: transparent; font-weight: 500; transition: background-color 150ms cubic-bezier(0.2, 0, 0, 1), transform 110ms cubic-bezier(0.2, 0, 0, 1), color 150ms ease; }\n"
		"button.nulla-pill-btn:hover { background-color: alpha(currentColor, 0.10); }\n"
		"button.nulla-pill-btn:active { background-color: alpha(currentColor, 0.18); transform: scale(0.93); }\n"
		"button.nulla-icon-pill { border-radius: 9999px; min-width: 34px; min-height: 34px; padding: 0; border: none; background-color: alpha(currentColor, 0.06); transition: background-color 150ms cubic-bezier(0.2, 0, 0, 1), transform 110ms cubic-bezier(0.2, 0, 0, 1); }\n"
		"button.nulla-icon-pill:hover { background-color: alpha(currentColor, 0.12); transform: scale(1.04); }\n"
		"button.nulla-icon-pill:active { background-color: alpha(currentColor, 0.20); transform: scale(0.91); }\n"
		"button.nulla-save-dirty { background-color: #2563eb; color: #ffffff; }\n"
		"button.nulla-save-dirty:hover { background-color: #1d4ed8; color: #ffffff; }\n"
		".nulla-title-pill { background-color: alpha(currentColor, 0.06); border-radius: 9999px; padding: 5px 16px; font-weight: 600; font-size: 0.94em; }\n"
		".nulla-wincontrols button { border-radius: 9999px; min-width: 28px; min-height: 28px; padding: 0; margin-left: 4px; background-color: alpha(currentColor, 0.06); border: none; transition: background-color 140ms ease, transform 100ms ease; }\n"
		".nulla-wincontrols button:hover { background-color: alpha(currentColor, 0.14); }\n"
		".nulla-wincontrols button:active { transform: scale(0.88); }\n";

	static const char * const css_widgets =
		"scrolledwindow.nulla-scroller { border-radius: 16px; margin: 2px 10px 6px 10px; border: 1px solid alpha(currentColor, 0.09); transition: border-color 180ms ease; }\n"
		"window.nulla-light scrolledwindow.nulla-scroller { background-color: #ffffff; }\n"
		"window.nulla-dark scrolledwindow.nulla-scroller { background-color: #181a20; }\n"
		"scrolledwindow.nulla-scroller:focus-within { border-color: alpha(currentColor, 0.26); }\n"
		"window.nulla-light .nulla-editor, window.nulla-light .nulla-editor text { background-color: #ffffff; }\n"
		"window.nulla-dark .nulla-editor, window.nulla-dark .nulla-editor text { background-color: #181a20; }\n"
		".nulla-statusbar { padding: 2px 12px 10px 12px; }\n"
		".nulla-chip { background-color: alpha(currentColor, 0.06); border-radius: 9999px; padding: 3px 11px; font-size: 0.84em; border: none; min-height: 22px; transition: background-color 140ms ease, transform 100ms ease; }\n"
		"button.nulla-chip:hover { background-color: alpha(currentColor, 0.12); }\n"
		"button.nulla-chip:active { transform: scale(0.93); }\n"
		"scrollbar { background: transparent; border: none; }\n"
		"scrollbar trough { background: transparent; border: none; }\n"
		"scrollbar slider { min-width: 6px; min-height: 6px; border-radius: 9999px; background-color: alpha(currentColor, 0.22); margin: 3px; transition: all 180ms cubic-bezier(0.2, 0, 0, 1); }\n"
		"scrollbar slider:hover { background-color: alpha(currentColor, 0.42); min-width: 9px; min-height: 9px; }\n"
		"scrollbar slider:active { background-color: alpha(currentColor, 0.62); }\n"
		"popover > contents { border-radius: 18px; padding: 6px; box-shadow: 0 8px 24px rgba(0, 0, 0, 0.22); border: 1px solid alpha(currentColor, 0.10); }\n"
		"popover modelbutton { border-radius: 12px; padding: 8px 14px; margin: 2px 0; transition: background-color 140ms ease, transform 100ms ease; }\n"
		"popover modelbutton:hover { background-color: alpha(currentColor, 0.10); }\n"
		"popover modelbutton:active { transform: scale(0.96); background-color: alpha(currentColor, 0.18); }\n"
		".nulla-settings-card { background-color: alpha(currentColor, 0.05); border-radius: 18px; padding: 18px 22px; border: 1px solid alpha(currentColor, 0.08); }\n"
		"togglebutton.nulla-m3-switch { border-radius: 9999px; min-width: 50px; min-height: 30px; padding: 0; border: 2px solid alpha(currentColor, 0.34); background-color: alpha(currentColor, 0.08); outline: none; transition: background-color 180ms cubic-bezier(0.2, 0, 0, 1), border-color 180ms cubic-bezier(0.2, 0, 0, 1), transform 110ms ease; }\n"
		"togglebutton.nulla-m3-switch:hover { background-color: alpha(currentColor, 0.14); }\n"
		"togglebutton.nulla-m3-switch:active { transform: scale(0.92); }\n"
		"togglebutton.nulla-m3-switch:checked { background-color: #2563eb; border-color: #2563eb; }\n"
		"togglebutton.nulla-m3-switch:checked:hover { background-color: #1d4ed8; border-color: #1d4ed8; }\n"
		"togglebutton.nulla-m3-switch .nulla-m3-thumb { border-radius: 9999px; min-width: 16px; min-height: 16px; margin-left: 5px; background-color: alpha(currentColor, 0.65); transition: margin-left 180ms cubic-bezier(0.2, 0, 0, 1), min-width 180ms cubic-bezier(0.2, 0, 0, 1), min-height 180ms cubic-bezier(0.2, 0, 0, 1), background-color 180ms ease; }\n"
		"togglebutton.nulla-m3-switch:checked .nulla-m3-thumb { min-width: 22px; min-height: 22px; margin-left: 22px; background-color: #ffffff; }\n"
		"dropdown > button { border-radius: 14px; padding: 6px 14px; border: 1px solid alpha(currentColor, 0.14); transition: all 140ms ease; }\n"
		"dropdown > button:hover { background-color: alpha(currentColor, 0.08); }\n"
		"dropdown > button:active { transform: scale(0.95); }\n"
		"spinbutton { border-radius: 14px; border: 1px solid alpha(currentColor, 0.14); padding: 2px; }\n"
		"spinbutton button { border-radius: 10px; min-width: 28px; min-height: 28px; transition: all 120ms ease; }\n"
		"spinbutton button:active { transform: scale(0.88); }\n";

	char *css = g_strdup_printf(
		"%s%s.nulla-editor { font-family: monospace; font-size: %spt; border-radius: 16px; }\n",
		css_base, css_widgets, buf);
	gtk_css_provider_load_from_string(win->css_provider, css);
	g_free(css);

	nulla_window_update_status(win);
}

void nulla_window_apply_settings(NullaWindow *win)
{
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(win->view),
	                                      win->config->show_line_numbers);
	nulla_window_apply_theme(win);
	nulla_window_apply_language(win);
	nulla_window_apply_zoom(win);
}

void nulla_window_update_title(NullaWindow *win)
{
	GFile *loc = gtk_source_file_get_location(win->source_file);
	gboolean modified = gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(win->buffer));
	char *name = loc != NULL ? g_file_get_basename(loc) : g_strdup("Untitled");
	char *title = g_strdup_printf("%s%s — Nulla", modified ? "*" : "", name);
	char *pill = g_strdup_printf("%s%s", modified ? "• " : "", name);

	gtk_window_set_title(GTK_WINDOW(win->window), title);
	if (win->title_label != NULL) {
		gtk_label_set_text(GTK_LABEL(win->title_label), pill);
	}
	if (win->save_btn != NULL) {
		if (modified) {
			gtk_widget_add_css_class(win->save_btn, "nulla-save-dirty");
		} else {
			gtk_widget_remove_css_class(win->save_btn, "nulla-save-dirty");
		}
	}

	g_free(pill);
	g_free(title);
	g_free(name);
}

void nulla_window_show_error(NullaWindow *win, const char *title, const char *message)
{
	GtkAlertDialog *alert = gtk_alert_dialog_new("%s", title);
	if (message != NULL) {
		gtk_alert_dialog_set_detail(alert, message);
	}
	gtk_alert_dialog_show(alert, GTK_WINDOW(win->window));
	g_object_unref(alert);
}

void nulla_window_run_pending(NullaWindow *win)
{
	NullaPendingAction action = win->pending_action;
	win->pending_action = NULLA_PENDING_NONE;

	switch (action) {
	case NULLA_PENDING_NEW:
		nulla_file_new(win);
		break;
	case NULLA_PENDING_OPEN:
		nulla_file_open_dialog(win);
		break;
	case NULLA_PENDING_CLOSE:
		win->force_close = TRUE;
		gtk_window_close(GTK_WINDOW(win->window));
		break;
	case NULLA_PENDING_NONE:
	default:
		break;
	}
}

static void on_gtk_theme_notify(GObject *obj, GParamSpec *pspec, gpointer user_data)
{
	(void)obj;
	(void)pspec;
	NullaWindow *win = user_data;
	if (win->config->theme_mode == NULLA_THEME_GTK) {
		nulla_window_update_source_scheme(win);
	}
}

static void on_buffer_modified(GtkTextBuffer *buf, gpointer user_data)
{
	(void)buf;
	NullaWindow *win = user_data;
	nulla_window_update_title(win);
}

static void on_cursor_position_notify(GObject *obj, GParamSpec *pspec, gpointer user_data)
{
	(void)obj;
	(void)pspec;
	nulla_window_update_status(user_data);
}

static gboolean on_close_request(GtkWindow *window, gpointer user_data)
{
	(void)window;
	NullaWindow *win = user_data;

	if (win->force_close) {
		return FALSE;
	}

	if (gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(win->buffer))) {
		nulla_file_confirm_discard(win, NULLA_PENDING_CLOSE);
		return TRUE;
	}

	return FALSE;
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data)
{
	(void)widget;
	NullaWindow *win = user_data;

	if (win->cancellable != NULL) {
		g_cancellable_cancel(win->cancellable);
		g_clear_object(&win->cancellable);
	}

	GtkSettings *gs = gtk_settings_get_default();
	if (gs != NULL) {
		g_signal_handlers_disconnect_by_data(gs, win);
	}

	if (win->css_provider != NULL) {
		gtk_style_context_remove_provider_for_display(
			gtk_widget_get_display(win->window),
			GTK_STYLE_PROVIDER(win->css_provider));
		g_clear_object(&win->css_provider);
	}

	g_clear_object(&win->source_file);
	g_free(win);
}

static void act_new(GSimpleAction *act, GVariant *param, gpointer user_data)
{
	(void)act;
	(void)param;
	nulla_file_new(user_data);
}

static void act_open(GSimpleAction *act, GVariant *param, gpointer user_data)
{
	(void)act;
	(void)param;
	nulla_file_open_dialog(user_data);
}

static void act_save(GSimpleAction *act, GVariant *param, gpointer user_data)
{
	(void)act;
	(void)param;
	nulla_file_save(user_data);
}

static void act_save_as(GSimpleAction *act, GVariant *param, gpointer user_data)
{
	(void)act;
	(void)param;
	nulla_file_save_as_dialog(user_data);
}

static void act_settings(GSimpleAction *act, GVariant *param, gpointer user_data)
{
	(void)act;
	(void)param;
	nulla_settings_show(user_data);
}

static void act_zoom_in(GSimpleAction *act, GVariant *param, gpointer user_data)
{
	(void)act;
	(void)param;
	NullaWindow *win = user_data;
	if (win->config->zoom_percent < 300) {
		win->config->zoom_percent += 10;
		if (win->config->zoom_percent > 300) {
			win->config->zoom_percent = 300;
		}
		nulla_window_apply_zoom(win);
		nulla_config_save(win->config);
	}
}

static void act_zoom_out(GSimpleAction *act, GVariant *param, gpointer user_data)
{
	(void)act;
	(void)param;
	NullaWindow *win = user_data;
	if (win->config->zoom_percent > 50) {
		win->config->zoom_percent -= 10;
		if (win->config->zoom_percent < 50) {
			win->config->zoom_percent = 50;
		}
		nulla_window_apply_zoom(win);
		nulla_config_save(win->config);
	}
}

static void act_zoom_reset(GSimpleAction *act, GVariant *param, gpointer user_data)
{
	(void)act;
	(void)param;
	NullaWindow *win = user_data;
	win->config->zoom_percent = 100;
	nulla_window_apply_zoom(win);
	nulla_config_save(win->config);
}

static GMenuModel *nulla_build_menu(void)
{
	GMenu *menu = g_menu_new();

	GMenu *file_sec = g_menu_new();
	g_menu_append(file_sec, "New", "win.new");
	g_menu_append(file_sec, "Open...", "win.open");
	g_menu_append(file_sec, "Save", "win.save");
	g_menu_append(file_sec, "Save As...", "win.save-as");
	g_menu_append_section(menu, NULL, G_MENU_MODEL(file_sec));
	g_object_unref(file_sec);

	GMenu *zoom_sec = g_menu_new();
	g_menu_append(zoom_sec, "Zoom In", "win.zoom-in");
	g_menu_append(zoom_sec, "Zoom Out", "win.zoom-out");
	g_menu_append(zoom_sec, "Reset Zoom", "win.zoom-reset");
	g_menu_append_section(menu, NULL, G_MENU_MODEL(zoom_sec));
	g_object_unref(zoom_sec);

	GMenu *app_sec = g_menu_new();
	g_menu_append(app_sec, "Settings", "win.settings");
	g_menu_append(app_sec, "Quit", "app.quit");
	g_menu_append_section(menu, NULL, G_MENU_MODEL(app_sec));
	g_object_unref(app_sec);

	return G_MENU_MODEL(menu);
}

static GtkWidget *nulla_build_topbar(NullaWindow *win)
{
	GtkWidget *handle = gtk_window_handle_new();
	GtkWidget *bar = gtk_center_box_new();
	gtk_widget_add_css_class(bar, "nulla-topbar");

	GtkWidget *left_group = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_widget_add_css_class(left_group, "nulla-pill-group");

	gtk_widget_set_valign(left_group, GTK_ALIGN_CENTER);

	GtkWidget *btn_open = gtk_button_new_with_label("Open");
	gtk_widget_set_valign(btn_open, GTK_ALIGN_CENTER);
	gtk_widget_add_css_class(btn_open, "nulla-pill-btn");
	gtk_widget_set_tooltip_text(btn_open, "Open File (Ctrl+O)");
	gtk_actionable_set_action_name(GTK_ACTIONABLE(btn_open), "win.open");
	gtk_box_append(GTK_BOX(left_group), btn_open);

	GtkWidget *btn_new = gtk_button_new_from_icon_name("document-new-symbolic");
	gtk_widget_set_valign(btn_new, GTK_ALIGN_CENTER);
	gtk_widget_add_css_class(btn_new, "nulla-pill-btn");
	gtk_widget_set_tooltip_text(btn_new, "New File (Ctrl+N)");
	gtk_actionable_set_action_name(GTK_ACTIONABLE(btn_new), "win.new");
	gtk_box_append(GTK_BOX(left_group), btn_new);

	GtkWidget *btn_save = gtk_button_new_with_label("Save");
	gtk_widget_set_valign(btn_save, GTK_ALIGN_CENTER);
	gtk_widget_add_css_class(btn_save, "nulla-pill-btn");
	gtk_widget_set_tooltip_text(btn_save, "Save File (Ctrl+S)");
	gtk_actionable_set_action_name(GTK_ACTIONABLE(btn_save), "win.save");
	gtk_box_append(GTK_BOX(left_group), btn_save);
	win->save_btn = btn_save;

	gtk_center_box_set_start_widget(GTK_CENTER_BOX(bar), left_group);

	win->title_label = gtk_label_new("Untitled");
	gtk_widget_add_css_class(win->title_label, "nulla-title-pill");
	gtk_label_set_ellipsize(GTK_LABEL(win->title_label), PANGO_ELLIPSIZE_MIDDLE);
	gtk_label_set_max_width_chars(GTK_LABEL(win->title_label), 28);
	gtk_center_box_set_center_widget(GTK_CENTER_BOX(bar), win->title_label);

	GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_set_valign(right_box, GTK_ALIGN_CENTER);

	GtkWidget *btn_settings = gtk_button_new_from_icon_name("emblem-system-symbolic");
	gtk_widget_set_valign(btn_settings, GTK_ALIGN_CENTER);
	gtk_widget_add_css_class(btn_settings, "nulla-icon-pill");
	gtk_widget_set_tooltip_text(btn_settings, "Settings (Ctrl+,)");
	gtk_actionable_set_action_name(GTK_ACTIONABLE(btn_settings), "win.settings");
	gtk_box_append(GTK_BOX(right_box), btn_settings);

	GtkWidget *btn_menu = gtk_menu_button_new();
	gtk_widget_set_valign(btn_menu, GTK_ALIGN_CENTER);
	gtk_widget_add_css_class(btn_menu, "nulla-icon-pill");
	gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(btn_menu), "view-more-symbolic");
	gtk_widget_set_tooltip_text(btn_menu, "Menu");
	GMenuModel *menu_model = nulla_build_menu();
	gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(btn_menu), menu_model);
	g_object_unref(menu_model);
	gtk_box_append(GTK_BOX(right_box), btn_menu);

	GtkWidget *win_controls = gtk_window_controls_new(GTK_PACK_END);
	gtk_widget_set_valign(win_controls, GTK_ALIGN_CENTER);
	gtk_widget_add_css_class(win_controls, "nulla-wincontrols");
	gtk_box_append(GTK_BOX(right_box), win_controls);

	gtk_center_box_set_end_widget(GTK_CENTER_BOX(bar), right_box);
	gtk_window_handle_set_child(GTK_WINDOW_HANDLE(handle), bar);
	return handle;
}

static GtkWidget *nulla_build_statusbar(NullaWindow *win)
{
	GtkWidget *status = gtk_center_box_new();
	gtk_widget_add_css_class(status, "nulla-statusbar");

	win->status_pos_label = gtk_label_new("Ln 1, Col 1");
	gtk_widget_add_css_class(win->status_pos_label, "nulla-chip");
	gtk_center_box_set_start_widget(GTK_CENTER_BOX(status), win->status_pos_label);

	GtkWidget *right = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

	GtkWidget *btn_lang = gtk_button_new();
	gtk_widget_add_css_class(btn_lang, "nulla-chip");
	gtk_widget_set_tooltip_text(btn_lang, "Change syntax language");
	win->status_lang_label = gtk_label_new("Plain");
	gtk_button_set_child(GTK_BUTTON(btn_lang), win->status_lang_label);
	gtk_actionable_set_action_name(GTK_ACTIONABLE(btn_lang), "win.settings");
	gtk_box_append(GTK_BOX(right), btn_lang);

	win->status_zoom_btn = gtk_button_new_with_label("100%");
	gtk_widget_add_css_class(win->status_zoom_btn, "nulla-chip");
	gtk_widget_set_tooltip_text(win->status_zoom_btn, "Reset zoom (Ctrl+0)");
	gtk_actionable_set_action_name(GTK_ACTIONABLE(win->status_zoom_btn), "win.zoom-reset");
	gtk_box_append(GTK_BOX(right), win->status_zoom_btn);

	gtk_center_box_set_end_widget(GTK_CENTER_BOX(status), right);
	return status;
}

NullaWindow *nulla_window_new(GtkApplication *app, NullaConfig *cfg)
{
	NullaWindow *win = g_new0(NullaWindow, 1);
	win->app = app;
	win->config = cfg;
	win->cancellable = g_cancellable_new();
	win->source_file = gtk_source_file_new();

	win->window = gtk_application_window_new(app);
	gtk_widget_add_css_class(win->window, "nulla-window");
	gtk_window_set_default_size(GTK_WINDOW(win->window), 820, 580);

	static const GActionEntry actions[] = {
		{ "new", act_new, NULL, NULL, NULL, {0} },
		{ "open", act_open, NULL, NULL, NULL, {0} },
		{ "save", act_save, NULL, NULL, NULL, {0} },
		{ "save-as", act_save_as, NULL, NULL, NULL, {0} },
		{ "settings", act_settings, NULL, NULL, NULL, {0} },
		{ "zoom-in", act_zoom_in, NULL, NULL, NULL, {0} },
		{ "zoom-out", act_zoom_out, NULL, NULL, NULL, {0} },
		{ "zoom-reset", act_zoom_reset, NULL, NULL, NULL, {0} }
	};
	g_action_map_add_action_entries(G_ACTION_MAP(win->window), actions,
	                                G_N_ELEMENTS(actions), win);

	gtk_window_set_titlebar(GTK_WINDOW(win->window), nulla_build_topbar(win));

	win->view = gtk_source_view_new();
	win->buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(win->view)));

	gtk_widget_add_css_class(win->view, "nulla-editor");
	gtk_text_view_set_monospace(GTK_TEXT_VIEW(win->view), TRUE);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(win->view), 12);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(win->view), 12);
	gtk_text_view_set_top_margin(GTK_TEXT_VIEW(win->view), 10);
	gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(win->view), 10);
	gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(win->view), TRUE);
	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(win->view), 4);
	gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(win->view), TRUE);

	GtkWidget *scroll = gtk_scrolled_window_new();
	gtk_widget_add_css_class(scroll, "nulla-scroller");
	gtk_widget_set_vexpand(scroll, TRUE);
	gtk_widget_set_hexpand(scroll, TRUE);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), win->view);

	GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_append(GTK_BOX(root), scroll);
	gtk_box_append(GTK_BOX(root), nulla_build_statusbar(win));
	gtk_window_set_child(GTK_WINDOW(win->window), root);

	win->css_provider = gtk_css_provider_new();
	gtk_style_context_add_provider_for_display(
		gtk_widget_get_display(win->window),
		GTK_STYLE_PROVIDER(win->css_provider),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	GtkSettings *gs = gtk_settings_get_default();
	if (gs != NULL) {
		g_signal_connect(gs, "notify::gtk-theme-name",
		                 G_CALLBACK(on_gtk_theme_notify), win);
#if GTK_CHECK_VERSION(4, 20, 0)
		g_signal_connect(gs, "notify::gtk-interface-color-scheme",
		                 G_CALLBACK(on_gtk_theme_notify), win);
#else
		g_signal_connect(gs, "notify::gtk-application-prefer-dark-theme",
		                 G_CALLBACK(on_gtk_theme_notify), win);
#endif
	}

	g_signal_connect(win->buffer, "modified-changed", G_CALLBACK(on_buffer_modified), win);
	g_signal_connect(win->buffer, "notify::cursor-position",
	                 G_CALLBACK(on_cursor_position_notify), win);
	g_signal_connect(win->window, "close-request", G_CALLBACK(on_close_request), win);
	g_signal_connect(win->window, "destroy", G_CALLBACK(on_window_destroy), win);

	nulla_window_apply_settings(win);
	nulla_window_update_title(win);

	gtk_window_present(GTK_WINDOW(win->window));
	gtk_widget_grab_focus(win->view);

	return win;
}
