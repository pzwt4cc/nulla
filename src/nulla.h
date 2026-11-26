#ifndef NULLA_H
#define NULLA_H

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

typedef enum {
	NULLA_THEME_GTK = 0,
	NULLA_THEME_LIGHT = 1,
	NULLA_THEME_DARK = 2
} NullaThemeMode;

typedef enum {
	NULLA_PENDING_NONE = 0,
	NULLA_PENDING_NEW,
	NULLA_PENDING_OPEN,
	NULLA_PENDING_CLOSE
} NullaPendingAction;

typedef struct {
	gboolean show_line_numbers;
	NullaThemeMode theme_mode;
	gboolean syntax_enabled;
	char *language_id;
	int zoom_percent;
} NullaConfig;

typedef struct {
	GtkApplication *app;
	GtkWidget *window;
	GtkWidget *view;
	GtkSourceBuffer *buffer;
	GtkSourceFile *source_file;
	GtkCssProvider *css_provider;
	GtkWidget *settings_window;
	NullaConfig *config;
	gboolean force_close;
	NullaPendingAction pending_action;
} NullaWindow;

void nulla_config_init(NullaConfig *cfg);
void nulla_config_free(NullaConfig *cfg);
void nulla_config_load(NullaConfig *cfg);
void nulla_config_save(const NullaConfig *cfg);
void nulla_settings_show(NullaWindow *win);

NullaWindow *nulla_window_new(GtkApplication *app, NullaConfig *cfg);
void nulla_window_apply_settings(NullaWindow *win);
void nulla_window_apply_language(NullaWindow *win);
void nulla_window_apply_theme(NullaWindow *win);
void nulla_window_apply_zoom(NullaWindow *win);
void nulla_window_update_title(NullaWindow *win);
void nulla_window_show_error(NullaWindow *win, const char *title, const char *message);
void nulla_window_run_pending(NullaWindow *win);

void nulla_file_new(NullaWindow *win);
void nulla_file_open_dialog(NullaWindow *win);
void nulla_file_load(NullaWindow *win, GFile *file, gboolean allow_missing);
void nulla_file_save(NullaWindow *win);
void nulla_file_save_as_dialog(NullaWindow *win);
void nulla_file_confirm_discard(NullaWindow *win, NullaPendingAction action);

#endif
