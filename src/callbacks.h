#include <gtk/gtk.h>


void
on_quit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_debugger_settings_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_add_breakpoint_menu_item_activate   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_continue_menu_item_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_step_over_menu_item_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_step_in_menu_item_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_step_out_menu_item_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_detach_debugger_menu_item_activate  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_stop_debugging_menu_item_activate   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_continue_button_clicked             (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_step_over_button_clicked            (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_step_in_button_clicked              (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_step_out_button_clicked             (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_detach_button_clicked               (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_toolbutton6_clicked                 (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_kill_button_clicked                 (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

gboolean
on_stack_view_select_cursor_row        (GtkTreeView     *treeview,
                                        gboolean         start_editing,
                                        gpointer         user_data);

void
on_preferences_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cancel_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_ok_clicked                          (GtkButton       *button,
                                        gpointer         user_data);

void
on_revert_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_var_view_select_cursor_row          (GtkTreeView     *treeview,
                                        gboolean         start_editing,
                                        gpointer         user_data);
