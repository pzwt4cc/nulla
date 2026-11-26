#include "nulla.h"

static void on_file_loaded(GObject *source, GAsyncResult *res,
                           gpointer user_data) {
  GError *err = NULL;

  if (!gtk_source_file_loader_load_finish(GTK_SOURCE_FILE_LOADER(source), res,
                                          &err)) {
    if (!g_error_matches(err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      NullaWindow *win = user_data;
      nulla_window_show_error(win, "Failed to open file", err->message);
    }
    g_error_free(err);
    return;
  }

  NullaWindow *win = user_data;
  gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(win->buffer), FALSE);
  nulla_window_apply_language(win);
  nulla_window_update_title(win);
}

void nulla_file_load(NullaWindow *win, GFile *file, gboolean allow_missing) {
  if (allow_missing && !g_file_query_exists(file, NULL)) {
    gtk_source_file_set_location(win->source_file, file);
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(win->buffer), "", 0);
    gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(win->buffer), FALSE);
    nulla_window_apply_language(win);
    nulla_window_update_title(win);
    return;
  }

  gtk_source_file_set_location(win->source_file, file);
  GtkSourceFileLoader *loader =
      gtk_source_file_loader_new(win->buffer, win->source_file);
  gtk_source_file_loader_load_async(loader, G_PRIORITY_DEFAULT,
                                    win->cancellable, NULL, NULL, NULL,
                                    on_file_loaded, win);
  g_object_unref(loader);
}

void nulla_file_new(NullaWindow *win) {
  if (gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(win->buffer))) {
    nulla_file_confirm_discard(win, NULLA_PENDING_NEW);
    return;
  }

  gtk_text_buffer_set_text(GTK_TEXT_BUFFER(win->buffer), "", 0);
  gtk_source_file_set_location(win->source_file, NULL);
  gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(win->buffer), FALSE);
  nulla_window_apply_language(win);
  nulla_window_update_title(win);
}

static void on_open_dialog_finish(GObject *source, GAsyncResult *res,
                                  gpointer user_data) {
  GError *err = NULL;
  GFile *file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), res, &err);

  if (file != NULL) {
    NullaWindow *win = user_data;
    nulla_file_load(win, file, FALSE);
    g_object_unref(file);
    return;
  }

  if (err != NULL) {
    if (!g_error_matches(err, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED) &&
        !g_error_matches(err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      NullaWindow *win = user_data;
      nulla_window_show_error(win, "Failed to open file", err->message);
    }
    g_error_free(err);
  }
}

void nulla_file_open_dialog(NullaWindow *win) {
  if (gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(win->buffer))) {
    nulla_file_confirm_discard(win, NULLA_PENDING_OPEN);
    return;
  }

  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, "Open File");

  GFile *current = gtk_source_file_get_location(win->source_file);
  if (current != NULL) {
    GFile *parent = g_file_get_parent(current);
    if (parent != NULL) {
      gtk_file_dialog_set_initial_folder(dialog, parent);
      g_object_unref(parent);
    }
  }

  gtk_file_dialog_open(dialog, GTK_WINDOW(win->window), win->cancellable,
                       on_open_dialog_finish, win);
  g_object_unref(dialog);
}

static void on_file_saved(GObject *source, GAsyncResult *res,
                          gpointer user_data) {
  GError *err = NULL;
  GtkSourceFileSaver *saver = GTK_SOURCE_FILE_SAVER(source);

  if (!gtk_source_file_saver_save_finish(saver, res, &err)) {
    if (!g_error_matches(err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      NullaWindow *win = user_data;
      win->pending_action = NULLA_PENDING_NONE;
      nulla_window_show_error(win, "Failed to save file", err->message);
    }
    g_error_free(err);
    return;
  }

  NullaWindow *win = user_data;
  GFile *target = gtk_source_file_saver_get_location(saver);
  if (target != NULL &&
      target != gtk_source_file_get_location(win->source_file)) {
    gtk_source_file_set_location(win->source_file, target);
  }
  gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(win->buffer), FALSE);
  nulla_window_apply_language(win);
  nulla_window_update_title(win);
  nulla_window_run_pending(win);
}

void nulla_file_save(NullaWindow *win) {
  GFile *loc = gtk_source_file_get_location(win->source_file);
  if (loc == NULL) {
    nulla_file_save_as_dialog(win);
    return;
  }

  GtkSourceFileSaver *saver =
      gtk_source_file_saver_new(win->buffer, win->source_file);
  gtk_source_file_saver_save_async(saver, G_PRIORITY_DEFAULT, win->cancellable,
                                   NULL, NULL, NULL, on_file_saved, win);
  g_object_unref(saver);
}

static void on_save_dialog_finish(GObject *source, GAsyncResult *res,
                                  gpointer user_data) {
  GError *err = NULL;
  GFile *file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), res, &err);

  if (file != NULL) {
    NullaWindow *win = user_data;
    GtkSourceFileSaver *saver = gtk_source_file_saver_new_with_target(
        win->buffer, win->source_file, file);
    gtk_source_file_saver_save_async(saver, G_PRIORITY_DEFAULT,
                                     win->cancellable, NULL, NULL, NULL,
                                     on_file_saved, win);
    g_object_unref(saver);
    g_object_unref(file);
    return;
  }

  if (err != NULL) {
    if (!g_error_matches(err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      NullaWindow *win = user_data;
      win->pending_action = NULLA_PENDING_NONE;
      if (!g_error_matches(err, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED)) {
        nulla_window_show_error(win, "Failed to save file", err->message);
      }
    }
    g_error_free(err);
  }
}

void nulla_file_save_as_dialog(NullaWindow *win) {
  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, "Save File");

  GFile *current = gtk_source_file_get_location(win->source_file);
  if (current != NULL) {
    gtk_file_dialog_set_initial_file(dialog, current);
  } else {
    gtk_file_dialog_set_initial_name(dialog, "Untitled");
  }

  gtk_file_dialog_save(dialog, GTK_WINDOW(win->window), win->cancellable,
                       on_save_dialog_finish, win);
  g_object_unref(dialog);
}

static void on_discard_response(GObject *source, GAsyncResult *res,
                                gpointer user_data) {
  GError *err = NULL;
  int choice =
      gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(source), res, &err);

  if (err != NULL) {
    if (!g_error_matches(err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      NullaWindow *win = user_data;
      win->pending_action = NULLA_PENDING_NONE;
    }
    g_error_free(err);
    return;
  }

  NullaWindow *win = user_data;
  if (choice == 1) {
    gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(win->buffer), FALSE);
    nulla_window_run_pending(win);
  } else if (choice == 2) {
    nulla_file_save(win);
  } else {
    win->pending_action = NULLA_PENDING_NONE;
  }
}

void nulla_file_confirm_discard(NullaWindow *win, NullaPendingAction action) {
  win->pending_action = action;

  GtkAlertDialog *alert = gtk_alert_dialog_new("Unsaved changes");
  gtk_alert_dialog_set_detail(alert, "Save changes to the current document?");

  const char *const buttons[] = {"Cancel", "Discard", "Save", NULL};
  gtk_alert_dialog_set_buttons(alert, buttons);
  gtk_alert_dialog_set_cancel_button(alert, 0);
  gtk_alert_dialog_set_default_button(alert, 2);

  gtk_alert_dialog_choose(alert, GTK_WINDOW(win->window), win->cancellable,
                          on_discard_response, win);
  g_object_unref(alert);
}
