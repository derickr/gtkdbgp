/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <gtk/gtk.h>
#include <gconf/gconf.h>

#include "callbacks.h"
#include "support.h"
#include "globals.h"
#include "getopt.h"

GtkWidget *MainWindow;
GtkWidget *AddBreakPointWindow;
GtkWidget *DebuggerSettingsWindow;

int main (int argc, char *argv[])
{
	GtkWidget *stack_view, *var_view, *breakpoint_view;
	GtkCellRenderer *r1, *r2;
	GtkTreeViewColumn *column1;
	GtkListStore *store, *var_store, *breakpoint_store;
	GtkTreeSortable *sortable;
	GtkTreeSelection *selection;
	GConfEngine *conf;
	int port, max_children, max_depth, max_string_length;
	gboolean break_on_warning;
	gchar *code_font, *var_font;

	/* Make default settings */
	conf = gconf_engine_get_default();
	port = gconf_engine_get_int(conf, "/apps/gtkdbgp/port", NULL);
	if (!port) {
		port = 9000;
		gconf_engine_set_int(conf, "/apps/gtkdbgp/port", port, NULL);
	}
	max_children = gconf_engine_get_int(conf, "/apps/gtkdbgp/max_children", NULL);
	if (!max_children) {
		max_children = 25;
		gconf_engine_set_int(conf, "/apps/gtkdbgp/max_children", max_children, NULL);
	}
	max_depth = gconf_engine_get_int(conf, "/apps/gtkdbgp/max_depth", NULL);
	if (!max_depth) {
		max_depth = 1;
		gconf_engine_set_int(conf, "/apps/gtkdbgp/max_depth", max_depth, NULL);
	}
	max_string_length = gconf_engine_get_int(conf, "/apps/gtkdbgp/max_string_length", NULL);
	if (!max_string_length) {
		max_string_length = 512;
		gconf_engine_set_int(conf, "/apps/gtkdbgp/max_string_length", max_string_length, NULL);
	}
	code_font = gconf_engine_get_string(conf, "/apps/gtkdbgp/font/code", NULL);
	if (!code_font) {
		code_font = "Monospace 9";
		gconf_engine_set_string(conf, "/apps/gtkdbgp/font/code", code_font, NULL);
	}
	var_font = gconf_engine_get_string(conf, "/apps/gtkdbgp/font/var", NULL);
	if (!var_font) {
		var_font = "Sans 8";
		gconf_engine_set_string(conf, "/apps/gtkdbgp/font/var", var_font, NULL);
	}
	break_on_warning = gconf_engine_get_bool(conf, "/apps/gtkdbgp/break_on_warning", NULL);
	gconf_engine_set_bool(conf, "/apps/gtkdbgp/break_on_warning", break_on_warning, NULL);

	/* Do command line arguments */
	{
		int c;
		int option_index = 0;

		while(1) {
			static struct option long_options[] = {
				{"help",  no_argument, 0, 'h'},
				{"port",  required_argument, 0, 'p'},
				{0, 0, 0, 0}
			};

			c = getopt_long(argc, argv, "hp:", long_options, &option_index);
			if (c == -1) {
				break;
			}

			switch(c) {
				case 'h':
					printf("\nHi! This is the standalone dbgp client in GTK!\n\n");
					printf("Supported options are:\n\n");
					printf("  -h, --help        This help\n\n");
					printf("  -p <portnr>       Selects a port number to listen to.\n");
					printf("  --port <portnr>   Example: --port 9002\n\n");
					exit(0);
				case 'p':
					port = strtol(optarg, NULL, 10);
					gconf_engine_set_int(conf, "/apps/gtkdbgp/port", port, NULL);
					break;
			}
		}
	}

	gconf_engine_unref(conf);

	gtk_set_locale();
	gtk_init(&argc, &argv);

//	add_pixmap_directory(PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");
	

	/*
	MainWindow = create_MainWindow();
	DebuggerSettingsWindow = create_DebuggerSettingsWindow();
	AddBreakPointWindow = create_AddBreakPointWindow();
*/
        
        builder = gtk_builder_new ();
        gtk_builder_add_from_file (builder, "gtkdbgp.xml", NULL);
 
//        window = GTK_WIDGET (gtk_builder_get_object (builder, "window"));

	gtk_builder_connect_signals(builder, NULL);

	MainWindow = GTK_WIDGET(gtk_builder_get_object(builder, "MainWindow"));
	gtk_widget_show(MainWindow);

//	g_signal_connect(MainWindow, "delete_event", gtk_main_quit, NULL);

	/* Create two renders */
 	r1 = gtk_cell_renderer_text_new();
	g_object_set(r1, "font-desc", pango_font_description_from_string (var_font), "xalign", 0.99999, NULL);
 	r2 = gtk_cell_renderer_text_new();
	g_object_set(r2, "font-desc", pango_font_description_from_string (var_font), NULL);

	/* Setup the stack view store */
	store = gtk_list_store_new(STACK_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

 	stack_view = GTK_WIDGET(gtk_builder_get_object(builder, "stack_view"));

	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(stack_view), -1, "#", r1, "text", STACK_NR_COLUMN, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(stack_view), -1, "Function", r2, "text", STACK_FUNCTION_COLUMN, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(stack_view), -1, "Location", r2, "text", STACK_LOCATION_COLUMN, NULL);

	gtk_tree_view_set_model(GTK_TREE_VIEW(stack_view), GTK_TREE_MODEL(store));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(stack_view));
	gtk_tree_selection_set_select_function(selection, stack_selection_function, NULL, NULL);


	/* Setup the breakpoint store */
	breakpoint_store = gtk_list_store_new(
		BREAKPOINT_N_COLUMNS,
		G_TYPE_INT, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_BOOLEAN);

	breakpoint_view = GTK_WIDGET(gtk_builder_get_object(builder, "breakpoint_view"));
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(breakpoint_view), -1, "ID", r1, "text", BREAKPOINT_ID_COLUMN, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(breakpoint_view), -1, "Enabled?", r2, "text", BREAKPOINT_ENABLED_COLUMN, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(breakpoint_view), -1, "Type", r2, "text", BREAKPOINT_TYPE_COLUMN, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(breakpoint_view), -1, "What", r2, "text", BREAKPOINT_WHAT_COLUMN, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(breakpoint_view), -1, "Hit Cond.", r2, "text", BREAKPOINT_HIT_CONDITION_COLUMN, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(breakpoint_view), -1, "Hit Value", r2, "text", BREAKPOINT_HIT_VALUE_COLUMN, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(breakpoint_view), -1, "Hit Count", r2, "text", BREAKPOINT_HIT_COUNT_COLUMN, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(breakpoint_view), -1, "Temp.", r2, "text", BREAKPOINT_TEMPORARY_COLUMN, NULL);

	gtk_tree_view_set_model(GTK_TREE_VIEW(breakpoint_view), GTK_TREE_MODEL(breakpoint_store));

	/* Setup the variables view store */
	var_store = GTK_LIST_STORE(gtk_tree_store_new(VARVIEW_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING));

	var_view = GTK_WIDGET(gtk_builder_get_object(builder, "var_view"));
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(var_view), -1, "Name", r2, "text", VARVIEW_NR_COLUMN, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(var_view), -1, "Type", r2, "text", VARVIEW_FUNCTION_COLUMN, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(var_view), -1, "Value", r2, "text", VARVIEW_LOCATION_COLUMN, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(var_view), -1, "H", r2, "text", VARVIEW_HIDDEN_HINT, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(var_view), -1, "PC", r2, "text", VARVIEW_PAGE_COUNT, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(var_view), -1, "PF", r2, "text", VARVIEW_PAGES_FETCHED, NULL);

	column1 = gtk_tree_view_get_column(GTK_TREE_VIEW(var_view), 0);
	g_object_set(column1, "resizable", 1, "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE, NULL);
	column1 = gtk_tree_view_get_column(GTK_TREE_VIEW(var_view), 1);
	g_object_set(column1, "resizable", 1, "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE, NULL);
	column1 = gtk_tree_view_get_column(GTK_TREE_VIEW(var_view), 2);
	g_object_set(column1, "resizable", 1, "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE, NULL);

	gtk_tree_view_set_model(GTK_TREE_VIEW(var_view), GTK_TREE_MODEL(var_store));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(var_view));
	gtk_tree_selection_set_select_function(selection, varview_selection_function, NULL, NULL);

	gtk_window_maximize(GTK_WINDOW(MainWindow));
//	gtk_widget_show(MainWindow);
//	gtk_builder_connect_signals (builder, NULL);          
//	g_object_unref (G_OBJECT (builder));

	start_server(port);

	gtk_main();
	return 0;
}

