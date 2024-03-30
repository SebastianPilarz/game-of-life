#include "game_of_life.h"
#include "gtk/gtkcssprovider.h"
#include <ggb/ggb.h>
#include <gnt/gnt.h>
#include <gtk/gtk.h>
#include <iostream>
#include <memory>
#include <string>

struct Data {
  GtkApplication *app = nullptr;
  GtkWindow *window = nullptr;
  GgbGrid *grid = nullptr;
  std::unique_ptr<GameOfLife> game = nullptr;
  bool running = false;
};

static void click_cb(GtkGestureClick *gesture, guint n_press, gdouble x,
                     gdouble y, gpointer user_data) {
  GgbGrid *grid =
      GGB_GRID(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture)));
  int xi, yi;
  ggb_grid_position_to_index(grid, x, y, &xi, &yi);
  g_print("click at %d, %d\n", xi, yi);
  // ggb_grid_toggle_at(grid, xi, yi, TRUE);
}

static int iterate_cb(gpointer user_data) {
  Data *data = (Data *)user_data;
  if (data->running) {
    data->game->iterate();
    GgbGrid *grid = GGB_GRID(gtk_window_get_child(data->window));
    for (int i = 0; i < data->game->get_cols_num(); i++) {
      for (int j = 0; j < data->game->get_rows_num(); j++) {
        if (data->game->get_cell(i, j)) {
          ggb_grid_set_at(grid, i, j, TRUE, FALSE);
        } else {
          ggb_grid_set_at(grid, i, j, FALSE, FALSE);
        }
      }
    }
    ggb_grid_redraw(grid);
  }
  return G_SOURCE_CONTINUE;
}

static void activate_cb(GtkApplication *app, gpointer user_data) {
  // window
  Data *data = (Data *)user_data;
  data->window = (GtkWindow *)gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(data->window), "Game of Life");
  gtk_window_set_default_size(GTK_WINDOW(data->window), 400, 400);

  // grid
  data->game = std::make_unique<GameOfLife>(100, 100);
  data->grid = ggb_grid_new();
  gtk_widget_set_hexpand(GTK_WIDGET(data->grid), TRUE);
  gtk_widget_set_vexpand(GTK_WIDGET(data->grid), TRUE);
  ggb_grid_set_size(data->grid, data->game->get_cols_num(),
                    data->game->get_rows_num());
  ggb_grid_set_cell_spacing(data->grid, 0);
  ggb_grid_set_draw_guidelines(data->grid, FALSE);
  gtk_window_set_child(data->window, GTK_WIDGET(data->grid));

  // grid click event
  GtkGesture *evk = gtk_gesture_click_new();
  g_signal_connect(evk, "pressed", G_CALLBACK(click_cb), NULL);
  gtk_widget_add_controller(GTK_WIDGET(data->grid), GTK_EVENT_CONTROLLER(evk));

  data->running = true;
  g_timeout_add(200, iterate_cb, data); // todo: add the id to data

  gtk_window_present(GTK_WINDOW(data->window));
  GntMacosWindow *const tweaker = gnt_macos_window_new(data->window);
  // GdkRGBA *color = new GdkRGBA();
  auto color = std::make_unique<GdkRGBA>();
  gdk_rgba_parse(color.get(), "rgb(35, 0, 35)");
  // gdk_rgba_parse(color.get(), "rgb(24,25,38)");
  gnt_macos_window_set_titlebar_color(tweaker, color.get());
  g_object_unref(tweaker);
}

int main() {
  gtk_init();
  GtkSettings *settings = gtk_settings_get_default();
  g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
  Data data;
  data.app = gtk_application_new("gnt.test.app", G_APPLICATION_DEFAULT_FLAGS);

  // css
  GtkCssProvider *css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_string(
      css_provider, "window.background { background-color: transparent; }");
  gtk_style_context_add_provider_for_display(
      gdk_display_get_default(), GTK_STYLE_PROVIDER(css_provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(css_provider);

  g_signal_connect(data.app, "activate", G_CALLBACK(activate_cb), &data);
  g_application_run(G_APPLICATION(data.app), 0, NULL);
  g_object_unref(data.app);
  return 0;
}