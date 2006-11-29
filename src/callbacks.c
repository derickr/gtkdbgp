#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gconf/gconf.h>
#include <string.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "globals.h"

extern ClientState *client_state;
extern GtkWidget* MainWindow;
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
	GtkNotebook *notebook;
	GtkEntry    *entry1, *entry2;
	int         col1, col2;
	GtkTreeView *breakpoint_view;
	GtkListStore *store;
	GtkTreeIter  iter;
	gchar       *display_string, *type;
	const gchar *string1, *string2;

	breakpoint_view = GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(MainWindow), "breakpoint_view"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(breakpoint_view));

	/* Figure out which tab was selected */
	notebook = GTK_NOTEBOOK(lookup_widget(GTK_WIDGET(AddBreakPointWindow), "breakpoint_type_notebook"));
	switch (gtk_notebook_get_current_page(notebook)) {
		case 0:
			/* file/line */
			entry1 = GTK_ENTRY(lookup_widget(GTK_WIDGET(AddBreakPointWindow), "bp_filename"));
			entry2 = GTK_ENTRY(lookup_widget(GTK_WIDGET(AddBreakPointWindow), "bp_linenumber"));
			string1 = gtk_entry_get_text(entry1);
			string2 = gtk_entry_get_text(entry2);
			type = "line";
			col1 = BREAKPOINT_FILE_NAME_COLUMN;
			col2 = BREAKPOINT_LINENO_COLUMN;
			display_string = xdebug_sprintf("%s:%s", string1, string2);
			break;
		case 1:
			/* class/function */
			entry1 = GTK_ENTRY(lookup_widget(GTK_WIDGET(AddBreakPointWindow), "bp_classname"));
			entry2 = GTK_ENTRY(lookup_widget(GTK_WIDGET(AddBreakPointWindow), "bp_functionname"));
			string1 = gtk_entry_get_text(entry1);
			string2 = gtk_entry_get_text(entry2);
			display_string = xdebug_sprintf("%s::%s", string1, string2);
			type = "call";
			col1 = BREAKPOINT_CLASS_NAME_COLUMN;
			col2 = BREAKPOINT_FUNCTION_NAME_COLUMN;
			break;
		case 2:
			/* exception */
			entry1 = GTK_ENTRY(lookup_widget(GTK_WIDGET(AddBreakPointWindow), "bp_exceptionname"));
			string1 = gtk_entry_get_text(entry1);
			string2 = "";
			display_string = xdstrdup(string1);
			type = "exception";
			col1 = BREAKPOINT_EXCEPTION_NAME_COLUMN;
			col2 = BREAKPOINT_LINENO_COLUMN; /* dummy */
			break;
	}

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
		BREAKPOINT_TYPE_COLUMN, type,
		BREAKPOINT_WHAT_COLUMN, display_string,
		col1, string1,
		col2, string2,
		BREAKPOINT_ENABLED_COLUMN, 1,
		BREAKPOINT_TEMPORARY_COLUMN, 0,
		BREAKPOINT_HIT_CONDITION_COLUMN, "",
		BREAKPOINT_HIT_VALUE_COLUMN, 0,
		BREAKPOINT_HIT_VALUE_COLUMN, 0,
		-1);
	xdfree(display_string);
	gtk_widget_hide(AddBreakPointWindow);
}

