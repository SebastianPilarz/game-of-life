#include "game_of_life.h"
#include "gtk/gtkcssprovider.h"
#include <adwaita.h>
#include <ggb/ggb.h>
#include <gnt/gnt.h>
#include <gtk/gtk.h>
#include <iostream>
#include <memory>
#include <string>

static void activate_cb(GtkApplication *app, gpointer user_data);
static int iterate_cb(gpointer user_data);
static void grid_click_cb(GtkGestureClick *gesture, guint n_press, gdouble x,
                          gdouble y, gpointer user_data);
static void play_btn_cb(GtkButton *button, gpointer user_data);
static void speed_slider_cb(GtkScale *scale, gpointer user_data);
static void debounce_timeout_cb(gpointer user_data);
static char *value_format_func(GtkScale *scale, gdouble value,
                               gpointer user_data);

constexpr auto PLAY_ICON = "media-playback-start-symbolic";
constexpr auto PAUSE_ICON = "media-playback-pause-symbolic";
constexpr auto APP_CSS = R"(
window.background {
  background-color: transparent;
}

.control-panel {
  padding: 10px;
}

.background {
  background-color: rgb(0, 12, 21);
}

.control-panel button {
  color: rgb(150, 150, 150);
}

.control-panel scale highlight {
  background-color: rgb(229, 145, 162);
}

.control-panel scale slider {
  background-color: rgb(20, 38, 50);
}

.control-panel label {
  color: rgb(150, 150, 150);
}
)";

struct Data {
  AdwApplication *app = nullptr;
  GtkWindow *window = nullptr;
  GgbGrid *grid = nullptr;
  GtkWidget *play_button = nullptr;
  GtkWidget *speed_slider = nullptr;
  GtkWidget *iteration_count_label = nullptr;
  std::unique_ptr<GameOfLife> game = nullptr;
  long iterate_timeout_id = -1;
  long slider_timeout_id = -1;
  bool running = false;
  int delay = 200;
};

static char *value_format_func(GtkScale *scale, gdouble value,
                               gpointer user_data) {
  return g_strdup_printf("%d ms", (int)value);
}

static void debounce_timeout_cb(gpointer user_data) {
  Data *data = (Data *)user_data;
  data->slider_timeout_id = -1;

  g_source_remove(data->iterate_timeout_id);
  data->delay = gtk_range_get_value(GTK_RANGE(data->speed_slider));
  data->iterate_timeout_id = g_timeout_add(data->delay, iterate_cb, data);
}

static void speed_slider_cb(GtkScale *scale, gpointer user_data) {
  Data *data = (Data *)user_data;
  if (data->slider_timeout_id > 0) {
    g_source_remove(data->slider_timeout_id);
  }

  data->slider_timeout_id = g_timeout_add_once(300, debounce_timeout_cb, data);
}

static void play_btn_cb(GtkButton *button, gpointer user_data) {
  Data *data = (Data *)user_data;
  data->running = !data->running;
  const gchar *icon_name = gtk_button_get_icon_name(button);
  if (g_strcmp0(icon_name, PLAY_ICON) == 0) {
    gtk_button_set_icon_name(button, PAUSE_ICON);
  } else {
    gtk_button_set_icon_name(button, PLAY_ICON);
  }
}

static void grid_click_cb(GtkGestureClick *gesture, guint n_press, gdouble x,
                          gdouble y, gpointer user_data) {
  Data *data = (Data *)user_data;
  GgbGrid *grid =
      GGB_GRID(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture)));
  int xi, yi;
  ggb_grid_position_to_index(grid, x, y, &xi, &yi);
  data->running = false;
  if (data->play_button) {
    gtk_button_set_icon_name(GTK_BUTTON(data->play_button), PLAY_ICON);
  }
  data->game->set_cell(xi, yi, !data->game->get_cell(xi, yi));
  ggb_grid_toggle_at(grid, xi, yi, TRUE);
}

static int iterate_cb(gpointer user_data) {
  Data *data = (Data *)user_data;
  if (data->running) {
    data->game->iterate();
    if (data->iteration_count_label) {
      const std::string label = "Iterations: <b>" +
                                std::to_string(data->game->get_iterations()) +
                                "</b>";
      gtk_label_set_markup(GTK_LABEL(data->iteration_count_label),
                           label.c_str());
    }
    for (int i = 0; i < data->game->get_cols_num(); i++) {
      for (int j = 0; j < data->game->get_rows_num(); j++) {
        if (data->game->get_cell(i, j)) {
          ggb_grid_set_at(data->grid, i, j, TRUE, FALSE);
        } else {
          ggb_grid_set_at(data->grid, i, j, FALSE, FALSE);
        }
      }
    }
    ggb_grid_redraw(data->grid);
  }
  return G_SOURCE_CONTINUE;
}

static void activate_cb(GtkApplication *app, gpointer user_data) {
  // window
  auto *data = (Data *)user_data;
  data->window = (GtkWindow *)gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(data->window), "Game of Life");
  gtk_window_set_default_size(GTK_WINDOW(data->window), 400, 450);
  auto vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child(data->window, vbox);

  // grid
  data->game = std::make_unique<GameOfLife>(100, 100);
  data->grid = ggb_grid_new();
  gtk_widget_set_size_request(GTK_WIDGET(data->grid), 400, 400);
  ggb_grid_set_size(data->grid, data->game->get_cols_num(),
                    data->game->get_rows_num());
  ggb_grid_set_cell_spacing(data->grid, 0);
  ggb_grid_set_draw_guidelines(data->grid, FALSE);
  gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(data->grid));

  // grid click event
  auto *evk = gtk_gesture_click_new();
  g_signal_connect(evk, "pressed", G_CALLBACK(grid_click_cb), data);
  gtk_widget_add_controller(GTK_WIDGET(data->grid), GTK_EVENT_CONTROLLER(evk));

  // control panel
  auto *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  // const char *hbox_classes[] = {"linked", NULL};
  const char *hbox_classes[] = {"control-panel", NULL};
  gtk_widget_set_css_classes(hbox, hbox_classes);
  // gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
  gtk_box_append(GTK_BOX(vbox), hbox);

  // play button
  data->play_button = gtk_button_new_from_icon_name(PAUSE_ICON);
  const char *play_button_classes[] = {"flat", "circular", NULL};
  gtk_widget_set_css_classes(data->play_button, play_button_classes);
  gtk_box_append(GTK_BOX(hbox), data->play_button);
  g_signal_connect(data->play_button, "clicked", G_CALLBACK(play_btn_cb), data);

  // speed slider
  data->speed_slider =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 5, 500, 1);
  gtk_widget_set_size_request(data->speed_slider, 100, -1);
  gtk_range_set_value(GTK_RANGE(data->speed_slider), data->delay);
  gtk_scale_set_draw_value(GTK_SCALE(data->speed_slider), TRUE);
  gtk_scale_set_format_value_func(GTK_SCALE(data->speed_slider),
                                  value_format_func, NULL, NULL);
  g_signal_connect(data->speed_slider, "value-changed",
                   G_CALLBACK(speed_slider_cb), data);
  gtk_box_append(GTK_BOX(hbox), data->speed_slider);

  // auto *filler = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  // gtk_widget_set_hexpand(filler, TRUE);
  // gtk_box_append(GTK_BOX(hbox), filler);

  // iteration counter
  data->iteration_count_label = gtk_label_new("");
  gtk_box_append(GTK_BOX(hbox), data->iteration_count_label);

  // iterate
  data->running = true;
  data->iterate_timeout_id = g_timeout_add(200, iterate_cb, data);

  gtk_window_present(GTK_WINDOW(data->window));
  // titlebar color
  const auto tweaker = gnt_macos_window_new(data->window);
  auto color = std::make_unique<GdkRGBA>();
  gdk_rgba_parse(color.get(), "rgb(0, 12, 21)");
  gnt_macos_window_set_titlebar_color(tweaker, color.get());
  g_object_unref(tweaker);
}

int main() {
  gtk_init();
  auto style_manager = adw_style_manager_get_default();
  adw_style_manager_set_color_scheme(style_manager,
                                     ADW_COLOR_SCHEME_FORCE_DARK);
  Data data;
  data.app =
      adw_application_new("game.of.life.app", G_APPLICATION_DEFAULT_FLAGS);

  // css
  auto *css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_string(css_provider, APP_CSS);
  gtk_style_context_add_provider_for_display(
      gdk_display_get_default(), GTK_STYLE_PROVIDER(css_provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(css_provider);

  g_signal_connect(data.app, "activate", G_CALLBACK(activate_cb), &data);
  g_application_run(G_APPLICATION(data.app), 0, NULL);
  if (data.iterate_timeout_id > 0) {
    g_source_remove(data.iterate_timeout_id);
    data.iterate_timeout_id = -1;
  }
  if (data.slider_timeout_id > 0) {
    g_source_remove(data.slider_timeout_id);
    data.slider_timeout_id = -1;
  }
  g_object_unref(data.app);
  return 0;
}