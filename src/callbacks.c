#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gconf/gconf.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "globals.h"

extern ClientState *client_state;
extern GtkWidget* DebuggerSettingsWindow;
extern GtkWidget* AddBreakPointWindow;

void
on_quit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtk_main_quit();
}


void
on_add_breakpoint_menu_item_activate   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gtk_widget_show(AddBreakPointWindow);
}


void
on_continue_menu_item_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_step_over_menu_item_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_step_in_menu_item_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_step_out_menu_item_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_detach_debugger_menu_item_activate  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_stop_debugging_menu_item_activate   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_about_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *aboutdialog;
	
	aboutdialog = create_aboutdialog ();
	gtk_widget_show (aboutdialog);
}


void
on_continue_button_clicked             (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
	process_continue_button(toolbutton, user_data);
}


void
on_step_over_button_clicked            (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
	process_step_over_button(toolbutton, user_data);
}


void
on_step_in_button_clicked              (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
	process_step_into_button(toolbutton, user_data);
}


void
on_step_out_button_clicked             (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
	process_step_out_button(toolbutton, user_data);
}


void
on_detach_button_clicked               (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
	process_detach_button(toolbutton, user_data);
}


void
on_kill_button_clicked                 (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
	process_kill_button(toolbutton, user_data);
}


gboolean
on_stack_view_select_cursor_row        (GtkTreeView     *treeview,
                                        gboolean         start_editing,
                                        gpointer         user_data)
{

  return FALSE;
}


void
on_preferences_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *PortWidget, *MaxDepth, *MaxStringLength, *MaxChildren, *PHPErrorBreak;
	int port, max_string_length;
	GConfEngine *conf;
	char small_buffer[32];

	/* Find ze widgetz! */
	PortWidget = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "port");
	MaxDepth = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "max_depth");
	MaxStringLength = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "max_string_length");
	MaxChildren = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "max_children");
	PHPErrorBreak = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "break_on_warning");

	/* Let's load the settings! */
	conf = gconf_engine_get_default();

	port = gconf_engine_get_int(conf, "/apps/gtkdbgp/port", NULL);
	snprintf(small_buffer, 31, "%u", port);
	gtk_entry_set_text(GTK_ENTRY(PortWidget), small_buffer);

	max_string_length = gconf_engine_get_int(conf, "/apps/gtkdbgp/max_string_length", NULL);
	snprintf(small_buffer, 31, "%u", max_string_length);
	gtk_entry_set_text(GTK_ENTRY(MaxStringLength), small_buffer);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(MaxChildren), gconf_engine_get_int(conf, "/apps/gtkdbgp/max_children", NULL));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(MaxDepth), gconf_engine_get_int(conf, "/apps/gtkdbgp/max_depth", NULL));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PHPErrorBreak), gconf_engine_get_bool(conf, "/apps/gtkdbgp/break_on_warning", NULL));

	gconf_engine_unref(conf);

	gtk_widget_show(DebuggerSettingsWindow);
}


void
on_cancel_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_hide(DebuggerSettingsWindow);
}


void
on_ok_clicked                          (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *PortWidget, *MaxDepth, *MaxStringLength, *MaxChildren, *PHPErrorBreak;
	int port, max_string_length;
	GConfEngine *conf;
	char small_buffer[32];

	gtk_widget_hide(DebuggerSettingsWindow);

	/* Find ze widgetz! */
	PortWidget = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "port");
	MaxDepth = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "max_depth");
	MaxStringLength = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "max_string_length");
	MaxChildren = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "max_children");
	PHPErrorBreak = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "break_on_warning");

	/* Let's save the settings! */
	conf = gconf_engine_get_default();

	gconf_engine_set_int(conf, "/apps/gtkdbgp/port", strtol(gtk_entry_get_text(GTK_ENTRY(PortWidget)), NULL, 10), NULL);
	gconf_engine_set_int(conf, "/apps/gtkdbgp/max_string_length", strtol(gtk_entry_get_text(GTK_ENTRY(MaxStringLength)), NULL, 10), NULL);
	gconf_engine_set_int(conf, "/apps/gtkdbgp/max_children", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(MaxChildren)), NULL);
	gconf_engine_set_int(conf, "/apps/gtkdbgp/max_depth", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(MaxDepth)), NULL);
	gconf_engine_set_bool(conf, "/apps/gtkdbgp/break_on_warning", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PHPErrorBreak)), NULL);

	gconf_engine_unref(conf);

}


void
on_revert_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *PortWidget, *MaxDepth, *MaxStringLength, *MaxChildren, *PHPErrorBreak;

	/* Find ze widgetz! */
	PortWidget = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "port");
	MaxDepth = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "max_depth");
	MaxStringLength = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "max_string_length");
	MaxChildren = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "max_children");
	PHPErrorBreak = lookup_widget(GTK_WIDGET(DebuggerSettingsWindow), "break_on_warning");

	gtk_entry_set_text(GTK_ENTRY(PortWidget), "9000");
	gtk_entry_set_text(GTK_ENTRY(MaxStringLength), "512");

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(MaxChildren), 25);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(MaxDepth), 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PHPErrorBreak), TRUE);
}


gboolean
on_var_view_select_cursor_row          (GtkTreeView     *treeview,
                                        gboolean         start_editing,
                                        gpointer         user_data)
{

  return FALSE;
}

void
on_main_add_bp_button_activate         (GtkButton       *button,
                                        gpointer         user_data)
{
	AddBreakPointWindow = create_AddBreakPointWindow();
	gtk_widget_show(AddBreakPointWindow);
}


void
on_main_edit_bp_button_activate        (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_show(AddBreakPointWindow);
}


void
on_main_remove_bp_button_activate      (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_show(AddBreakPointWindow);
}


void
on_add_bp_cancel_button_activate       (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_hide(AddBreakPointWindow);
}


void
on_add_bp_add_button_activate          (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_hide(AddBreakPointWindow);
}

