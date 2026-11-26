#include "nulla.h"

typedef struct {
  NullaConfig cfg;
} NullaAppState;

static void act_quit(GSimpleAction *act, GVariant *param, gpointer user_data) {
  (void)act;
  (void)param;
  GtkApplication *app = GTK_APPLICATION(user_data);
  GList *wins = g_list_copy(gtk_application_get_windows(app));
  for (GList *l = wins; l != NULL; l = l->next) {
    gtk_window_close(GTK_WINDOW(l->data));
  }
  g_list_free(wins);
}

static void on_startup(GApplication *gapp, gpointer user_data) {
  NullaAppState *state = user_data;
  GtkApplication *app = GTK_APPLICATION(gapp);

  gtk_source_init();
  nulla_config_init(&state->cfg);
  nulla_config_load(&state->cfg);

  static const GActionEntry app_actions[] = {
      {"quit", act_quit, NULL, NULL, NULL, {0}}};
  g_action_map_add_action_entries(G_ACTION_MAP(app), app_actions,
                                  G_N_ELEMENTS(app_actions), app);

  const char *const accels_new[] = {"<Control>n", NULL};
  const char *const accels_open[] = {"<Control>o", NULL};
  const char *const accels_save[] = {"<Control>s", NULL};
  const char *const accels_save_as[] = {"<Control><Shift>s", NULL};
  const char *const accels_settings[] = {"<Control>comma", NULL};
  const char *const accels_zoom_in[] = {"<Control>plus", "<Control>equal",
                                        "<Control>KP_Add", NULL};
  const char *const accels_zoom_out[] = {"<Control>minus",
                                         "<Control>KP_Subtract", NULL};
  const char *const accels_zoom_reset[] = {"<Control>0", "<Control>KP_0", NULL};
  const char *const accels_quit[] = {"<Control>q", NULL};

  gtk_application_set_accels_for_action(app, "win.new", accels_new);
  gtk_application_set_accels_for_action(app, "win.open", accels_open);
  gtk_application_set_accels_for_action(app, "win.save", accels_save);
  gtk_application_set_accels_for_action(app, "win.save-as", accels_save_as);
  gtk_application_set_accels_for_action(app, "win.settings", accels_settings);
  gtk_application_set_accels_for_action(app, "win.zoom-in", accels_zoom_in);
  gtk_application_set_accels_for_action(app, "win.zoom-out", accels_zoom_out);
  gtk_application_set_accels_for_action(app, "win.zoom-reset",
                                        accels_zoom_reset);
  gtk_application_set_accels_for_action(app, "app.quit", accels_quit);
}

static void on_activate(GApplication *gapp, gpointer user_data) {
  NullaAppState *state = user_data;
  nulla_window_new(GTK_APPLICATION(gapp), &state->cfg);
}

static void on_open(GApplication *gapp, GFile **files, int n_files,
                    const char *hint, gpointer user_data) {
  (void)hint;
  NullaAppState *state = user_data;
  GtkApplication *app = GTK_APPLICATION(gapp);

  for (int i = 0; i < n_files; i++) {
    NullaWindow *win = nulla_window_new(app, &state->cfg);
    nulla_file_load(win, files[i], TRUE);
  }
}

static void on_shutdown(GApplication *gapp, gpointer user_data) {
  (void)gapp;
  NullaAppState *state = user_data;
  nulla_config_free(&state->cfg);
  gtk_source_finalize();
}

int main(int argc, char **argv) {
  NullaAppState state = {0};
  GtkApplication *app = gtk_application_new(
      "org.nulla.Editor", (GApplicationFlags)(G_APPLICATION_HANDLES_OPEN |
                                              G_APPLICATION_NON_UNIQUE));

  g_signal_connect(app, "startup", G_CALLBACK(on_startup), &state);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), &state);
  g_signal_connect(app, "open", G_CALLBACK(on_open), &state);
  g_signal_connect(app, "shutdown", G_CALLBACK(on_shutdown), &state);

  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}
