#include "nulla.h"

#include <string.h>

static gboolean nulla_gtk_is_dark(void) {
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
#endif

  gboolean prefer_dark = FALSE;
  g_object_get(gs, "gtk-application-prefer-dark-theme", &prefer_dark, NULL);
  if (prefer_dark) {
    return TRUE;
  }

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

static void nulla_window_update_source_scheme(NullaWindow *win) {
  gboolean dark = FALSE;

  if (win->config->theme_mode == NULLA_THEME_DARK) {
    dark = TRUE;
  } else if (win->config->theme_mode == NULLA_THEME_LIGHT) {
    dark = FALSE;
  } else {
    dark = nulla_gtk_is_dark();
  }

  GtkSourceStyleSchemeManager *sm =
      gtk_source_style_scheme_manager_get_default();
  GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(
      sm, dark ? "Adwaita-dark" : "Adwaita");
  if (scheme == NULL) {
    scheme = gtk_source_style_scheme_manager_get_scheme(sm, dark ? "oblivion"
                                                                 : "classic");
  }
  gtk_source_buffer_set_style_scheme(win->buffer, scheme);
}

void nulla_window_apply_theme(NullaWindow *win) {
  GtkSettings *gs = gtk_settings_get_default();

  if (gs != NULL) {
    if (win->config->theme_mode == NULLA_THEME_LIGHT) {
      g_object_set(gs, "gtk-application-prefer-dark-theme", FALSE, NULL);
#if GTK_CHECK_VERSION(4, 20, 0)
      g_object_set(gs, "gtk-interface-color-scheme",
                   GTK_INTERFACE_COLOR_SCHEME_LIGHT, NULL);
#endif
    } else if (win->config->theme_mode == NULLA_THEME_DARK) {
      g_object_set(gs, "gtk-application-prefer-dark-theme", TRUE, NULL);
#if GTK_CHECK_VERSION(4, 20, 0)
      g_object_set(gs, "gtk-interface-color-scheme",
                   GTK_INTERFACE_COLOR_SCHEME_DARK, NULL);
#endif
    } else {
      gtk_settings_reset_property(gs, "gtk-application-prefer-dark-theme");
#if GTK_CHECK_VERSION(4, 20, 0)
      gtk_settings_reset_property(gs, "gtk-interface-color-scheme");
#endif
    }
  }

  nulla_window_update_source_scheme(win);
}

void nulla_window_apply_language(NullaWindow *win) {
  if (!win->config->syntax_enabled) {
    gtk_source_buffer_set_highlight_syntax(win->buffer, FALSE);
    gtk_source_buffer_set_language(win->buffer, NULL);
    return;
  }

  GtkSourceLanguageManager *lm = gtk_source_language_manager_get_default();
  GtkSourceLanguage *lang = NULL;

  if (win->config->language_id != NULL &&
      g_strcmp0(win->config->language_id, "auto") != 0) {
    lang =
        gtk_source_language_manager_get_language(lm, win->config->language_id);
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
}

void nulla_window_apply_zoom(NullaWindow *win) {
  double pt = 11.0 * ((double)win->config->zoom_percent / 100.0);
  char buf[G_ASCII_DTOSTR_BUF_SIZE];
  g_ascii_formatd(buf, sizeof(buf), "%.2f", pt);

  char *css = g_strdup_printf(
      ".nulla-editor { font-family: monospace; font-size: %spt; }", buf);
  gtk_css_provider_load_from_string(win->css_provider, css);
  g_free(css);
}

void nulla_window_apply_settings(NullaWindow *win) {
  gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(win->view),
                                        win->config->show_line_numbers);
  nulla_window_apply_theme(win);
  nulla_window_apply_language(win);
  nulla_window_apply_zoom(win);
}

void nulla_window_update_title(NullaWindow *win) {
  GFile *loc = gtk_source_file_get_location(win->source_file);
  gboolean modified =
      gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(win->buffer));
  char *name = loc != NULL ? g_file_get_basename(loc) : g_strdup("Untitled");
  char *title = g_strdup_printf("%s%s — Nulla", modified ? "*" : "", name);

  gtk_window_set_title(GTK_WINDOW(win->window), title);

  g_free(title);
  g_free(name);
}

void nulla_window_show_error(NullaWindow *win, const char *title,
                             const char *message) {
  GtkAlertDialog *alert = gtk_alert_dialog_new("%s", title);
  if (message != NULL) {
    gtk_alert_dialog_set_detail(alert, message);
  }
  gtk_alert_dialog_show(alert, GTK_WINDOW(win->window));
  g_object_unref(alert);
}

void nulla_window_run_pending(NullaWindow *win) {
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

static void on_gtk_theme_notify(GObject *obj, GParamSpec *pspec,
                                gpointer user_data) {
  (void)obj;
  (void)pspec;
  NullaWindow *win = user_data;
  if (win->config->theme_mode == NULLA_THEME_GTK) {
    nulla_window_update_source_scheme(win);
  }
}

static void on_buffer_modified(GtkTextBuffer *buf, gpointer user_data) {
  (void)buf;
  NullaWindow *win = user_data;
  nulla_window_update_title(win);
}

static gboolean on_close_request(GtkWindow *window, gpointer user_data) {
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

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
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

static void act_new(GSimpleAction *act, GVariant *param, gpointer user_data) {
  (void)act;
  (void)param;
  nulla_file_new(user_data);
}

static void act_open(GSimpleAction *act, GVariant *param, gpointer user_data) {
  (void)act;
  (void)param;
  nulla_file_open_dialog(user_data);
}

static void act_save(GSimpleAction *act, GVariant *param, gpointer user_data) {
  (void)act;
  (void)param;
  nulla_file_save(user_data);
}

static void act_save_as(GSimpleAction *act, GVariant *param,
                        gpointer user_data) {
  (void)act;
  (void)param;
  nulla_file_save_as_dialog(user_data);
}

static void act_settings(GSimpleAction *act, GVariant *param,
                         gpointer user_data) {
  (void)act;
  (void)param;
  nulla_settings_show(user_data);
}

static void act_zoom_in(GSimpleAction *act, GVariant *param,
                        gpointer user_data) {
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

static void act_zoom_out(GSimpleAction *act, GVariant *param,
                         gpointer user_data) {
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

static void act_zoom_reset(GSimpleAction *act, GVariant *param,
                           gpointer user_data) {
  (void)act;
  (void)param;
  NullaWindow *win = user_data;
  win->config->zoom_percent = 100;
  nulla_window_apply_zoom(win);
  nulla_config_save(win->config);
}

static GMenuModel *nulla_build_menu(void) {
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

NullaWindow *nulla_window_new(GtkApplication *app, NullaConfig *cfg) {
  NullaWindow *win = g_new0(NullaWindow, 1);
  win->app = app;
  win->config = cfg;
  win->cancellable = g_cancellable_new();
  win->source_file = gtk_source_file_new();

  win->window = gtk_application_window_new(app);
  gtk_window_set_default_size(GTK_WINDOW(win->window), 760, 540);

  static const GActionEntry actions[] = {
      {"new", act_new, NULL, NULL, NULL, {0}},
      {"open", act_open, NULL, NULL, NULL, {0}},
      {"save", act_save, NULL, NULL, NULL, {0}},
      {"save-as", act_save_as, NULL, NULL, NULL, {0}},
      {"settings", act_settings, NULL, NULL, NULL, {0}},
      {"zoom-in", act_zoom_in, NULL, NULL, NULL, {0}},
      {"zoom-out", act_zoom_out, NULL, NULL, NULL, {0}},
      {"zoom-reset", act_zoom_reset, NULL, NULL, NULL, {0}}};
  g_action_map_add_action_entries(G_ACTION_MAP(win->window), actions,
                                  G_N_ELEMENTS(actions), win);

  GtkWidget *header = gtk_header_bar_new();
  gtk_window_set_titlebar(GTK_WINDOW(win->window), header);

  GtkWidget *btn_open = gtk_button_new_from_icon_name("document-open-symbolic");
  gtk_widget_set_tooltip_text(btn_open, "Open (Ctrl+O)");
  gtk_actionable_set_action_name(GTK_ACTIONABLE(btn_open), "win.open");
  gtk_header_bar_pack_start(GTK_HEADER_BAR(header), btn_open);

  GtkWidget *btn_menu = gtk_menu_button_new();
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(btn_menu),
                                "open-menu-symbolic");
  gtk_widget_set_tooltip_text(btn_menu, "Menu");
  GMenuModel *menu_model = nulla_build_menu();
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(btn_menu), menu_model);
  g_object_unref(menu_model);
  gtk_header_bar_pack_end(GTK_HEADER_BAR(header), btn_menu);

  GtkWidget *btn_save = gtk_button_new_from_icon_name("document-save-symbolic");
  gtk_widget_set_tooltip_text(btn_save, "Save (Ctrl+S)");
  gtk_actionable_set_action_name(GTK_ACTIONABLE(btn_save), "win.save");
  gtk_header_bar_pack_end(GTK_HEADER_BAR(header), btn_save);

  win->view = gtk_source_view_new();
  win->buffer =
      GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(win->view)));

  gtk_widget_add_css_class(win->view, "nulla-editor");
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(win->view), TRUE);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(win->view), 6);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(win->view), 6);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(win->view), 6);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(win->view), 6);
  gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(win->view), TRUE);
  gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(win->view), 4);
  gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(win->view), TRUE);

  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), win->view);
  gtk_window_set_child(GTK_WINDOW(win->window), scroll);

  win->css_provider = gtk_css_provider_new();
  gtk_style_context_add_provider_for_display(
      gtk_widget_get_display(win->window),
      GTK_STYLE_PROVIDER(win->css_provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  GtkSettings *gs = gtk_settings_get_default();
  if (gs != NULL) {
    g_signal_connect(gs, "notify::gtk-application-prefer-dark-theme",
                     G_CALLBACK(on_gtk_theme_notify), win);
    g_signal_connect(gs, "notify::gtk-theme-name",
                     G_CALLBACK(on_gtk_theme_notify), win);
#if GTK_CHECK_VERSION(4, 20, 0)
    g_signal_connect(gs, "notify::gtk-interface-color-scheme",
                     G_CALLBACK(on_gtk_theme_notify), win);
#endif
  }

  g_signal_connect(win->buffer, "modified-changed",
                   G_CALLBACK(on_buffer_modified), win);
  g_signal_connect(win->window, "close-request", G_CALLBACK(on_close_request),
                   win);
  g_signal_connect(win->window, "destroy", G_CALLBACK(on_window_destroy), win);

  nulla_window_apply_settings(win);
  nulla_window_update_title(win);

  gtk_window_present(GTK_WINDOW(win->window));
  gtk_widget_grab_focus(win->view);

  return win;
}
