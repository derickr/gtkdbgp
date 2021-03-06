/* Echo server (TCP blocking)
 * Copyright (C) 2000-2003  David Helder
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <gnet.h>

#include <signal.h>
#include <gtk/gtk.h>
#include <gconf/gconf.h>

#include "support.h"
#include "usefulstuff.h"
#include "xdebug_hash.h"
#include "xdebug_xml.h"

#include "globals.h"

#define JMP_MASK                       0x00ff
#define JMP_IF_BREAK_ON_ERROR_FEATURE  0x0100

#define DBGP_PHP_NOTICE      1
#define DBGP_PHP_WARNING     2
#define DBGP_PHP_ERROR       3
#define DBGP_PHP_STRICT      4

#define FEATURE_MAX_CHILDREN 1
#define FEATURE_MAX_DATA     2
#define FEATURE_MAX_DEPTH    3

enum
{
	SOURCE_BP_COLUMN = 0,
	SOURCE_ST_COLUMN,
	SOURCE_LINENO_COLUMN,
	SOURCE_LINE_COLUMN,
	SOURCE_N_COLUMNS
};

ClientState* client_state;

extern GtkWidget *MainWindow;

static void clientstate_delete (ClientState* state);

static void async_accept (GTcpSocket* server_socket, GTcpSocket* client_socket, gpointer data);
static gboolean async_client_iofunc (GIOChannel* client, GIOCondition condition, gpointer data);

static void async_sig_int (int signum);
void update_statusbar(gchar* text);

static GTcpSocket* async_server = NULL;
static xdebug_hash *code_tabs_hash;

static void add_property(GtkTreeStore *store, GtkTreeIter *parent_iter, xdebug_xml_node *property);
void add_source_file(gchar* filename, gchar *source);

int do_context_get(int param, ClientState* state);
int do_run(int param, ClientState* state);
int do_set_error_breakpoint(int param, ClientState* state);
int do_set_feature(int param, ClientState* state);
int do_source_get(int param, ClientState* state);
int do_stack_get(int param, ClientState* state);
int do_step_into(int param, ClientState* state);

int process_add_source_file(int param, ClientState* state);
int process_fetched_property(int param, ClientState* state);
int process_jump(int param, ClientState* state);
int process_mark_buttons_active(int param, ClientState* state);
int process_mark_buttons_inactive(int param, ClientState* state);
int process_select_file_line_for_stack_0(int param, ClientState* state);
int process_select_file_line_for_stack_0_no_check(int param, ClientState* state);
int process_select_file_line_for_selected_stack(int param, ClientState* state);
int process_update_context_vars_for_selected_stack(int param, ClientState* state);
int process_update_stack(int param, ClientState* state);

#define SERVER_STATE_INPUT             0
#define SERVER_STATE_INITIAL           1
#define SERVER_STATE_BREAK             2
#define SERVER_STATE_STOPPED           3
#define SERVER_STATE_SELECT_STACK      4
#define SERVER_STATE_FETCH_PROPERTY    5

#define SERVER_ACTION_SOURCE_GET       1
#define SERVER_ACTION_STEP_IN          2
#define SERVER_ACTION_STACK_GET        3
#define SERVER_ACTION_CONTEXT_GET      4

#define INTERACTIVE                    0
#define NON_INTERACTIVE                1

int do_context_get(int param, ClientState* state)
{
	state->command = xdebug_sprintf( "context_get -i %d -d %d", get_next_id(state), state->last_selected_stack_frame);
	return INTERACTIVE;
}


int do_run(int param, ClientState* state)
{
	state->command = xdebug_sprintf( "run -i %d", get_next_id(state));
	return INTERACTIVE;
}


int do_set_error_breakpoint(int param, ClientState* state)
{
	char *name;

	switch (param) {
		case DBGP_PHP_NOTICE: name = "Notice"; break;
		case DBGP_PHP_WARNING: name = "Warning"; break;
		case DBGP_PHP_ERROR: name = "Error"; break;
		case DBGP_PHP_STRICT: name = "Strict"; break;
	}
	state->command = xdebug_sprintf( "breakpoint_set -i %d -t exception -x \"%s\"", get_next_id(state), name);
	return INTERACTIVE;
}


int do_set_feature(int param, ClientState* state)
{
	char *name, *key;
	GConfEngine *conf;
	gint value;

	conf = gconf_engine_get_default();

	switch (param) {
		case FEATURE_MAX_CHILDREN: name = "max_children"; key = "/apps/gtkdbgp/max_children"; break;
		case FEATURE_MAX_DATA:     name = "max_data";     key = "/apps/gtkdbgp/max_string_length"; break;
		case FEATURE_MAX_DEPTH:    name = "max_depth";    key = "/apps/gtkdbgp/max_depth"; break;
	}
	value = gconf_engine_get_int(conf, key, NULL);
	state->command = xdebug_sprintf( "feature_set -i %d -n %s -v %d", get_next_id(state), name, value);

	gconf_engine_unref(conf);
	return INTERACTIVE;
}


int do_source_get(int param, ClientState* state)
{
	state->command = xdebug_sprintf( "source -i %d -f %s", get_next_id(state), state->last_source_file);
	return INTERACTIVE;
}

int do_property_get(int param, ClientState* state)
{
	if (param) {
		state->command = xdebug_sprintf( "property_get -i %d -n %s -d %d -p %d", get_next_id(state), state->property, state->last_selected_stack_frame, param);
	} else {
		state->command = xdebug_sprintf( "property_get -i %d -n %s -d %d", get_next_id(state), state->property, state->last_selected_stack_frame);
	}
	xdfree(state->property);
	return INTERACTIVE;
}


int do_stack_get(int param, ClientState* state)
{
	state->command = xdebug_sprintf( "stack_get -i %d", get_next_id(state));
	return INTERACTIVE;
}


int do_step_into(int param, ClientState* state)
{
	state->command = xdebug_sprintf( "step_into -i %d", get_next_id(state));
	return INTERACTIVE;
}



int process_add_source_file(int param, ClientState* state)
{
	xdebug_xml_node *message = state->message;

	add_source_file(state->last_source_file, message->text->text);
	xdfree(state->last_source_file);
	state->last_source_file = NULL;
	return NON_INTERACTIVE;
}


int process_fetched_property(int param, ClientState* state)
{
	GtkWidget *var_view;
	GtkTreeStore *store;
	xdebug_xml_node *property;
	GtkTreeIter iter;

 	var_view = GTK_WIDGET(gtk_builder_get_object(builder, "var_view"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(var_view)));

	property = state->message;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, client_state->path_string);
	add_property(store, &iter, property->child);
	gtk_tree_view_expand_row(GTK_TREE_VIEW(var_view), gtk_tree_path_new_from_string(client_state->path_string), 0);
	g_free(client_state->path_string);

	return NON_INTERACTIVE;
}


int process_jump(int param, ClientState* state)
{
	GConfEngine *conf;

	conf = gconf_engine_get_default();
	if (!gconf_engine_get_bool(conf, "/apps/gtkdbgp/break_on_warning", NULL)) {
		state->action_list_ptr += (param & JMP_MASK);
	}
	gconf_engine_unref(conf);

	return NON_INTERACTIVE;
}


int process_mark_buttons_active(int param, ClientState* state)
{
	update_statusbar( "Waiting for input." );
	state->buttons_active = TRUE;
	return NON_INTERACTIVE;
}


int process_mark_buttons_inactive(int param, ClientState* state)
{
	state->buttons_active = FALSE;
	update_statusbar( "Processing..." );
	return NON_INTERACTIVE;
}


static int _process_select_file_line_for_stack_0(int param, ClientState* state, int check)
{
	/* Let's find the page! */
	/* - first we figure out stack frame 0, and the filename from it */
	xdebug_xml_node *message = state->message;
	xdebug_xml_attribute *filename_attr, *lineno_attr;
	dbgp_code_page *page;
	GtkWidget *code_notebook = GTK_WIDGET(gtk_builder_get_object(builder, "code_notebook"));
	GtkTreeSelection *selection;
	GtkTreePath      *path;
	GtkListStore     *store;
	GtkTreeIter       iter;

	if (message->child && strcmp(message->child->tag, "stack") == 0) {
		filename_attr = xdebug_xml_fetch_attribute(message->child, "filename");
		lineno_attr = xdebug_xml_fetch_attribute(message->child, "lineno");

		/* Now we have the filename in attr->value */
		if (xdebug_hash_find(code_tabs_hash, filename_attr->value, strlen(filename_attr->value), (void*) &page)) {
			/* If we found it then we advance the action pointer 4 items so that we
			 * don't call an add_source for it, but only the first time. */
			if (check) {
				state->action_list_ptr += param;
			}

			/* Use the old iter to unset the pixbuf */
			if (state->last_store) {
				gtk_list_store_set(state->last_store, &state->last_iter,
					SOURCE_ST_COLUMN, NULL,
					-1);
			}

			/* then we select the page */
			gtk_notebook_set_current_page(GTK_NOTEBOOK(code_notebook), page->nr);
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(page->tree));
			path = gtk_tree_path_new_from_indices(atoi(lineno_attr->value) - 1, -1);

			/* Figure out the iter for the path */
			store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(page->tree)));
			gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);

			/* Remember the iter for later use */
			state->last_iter = iter;
			state->last_store = GTK_LIST_STORE(store);

			/* Set the new pixbuf */
			gtk_list_store_set(GTK_LIST_STORE(store), &iter,
				SOURCE_ST_COLUMN, RunPixbuf,
				-1);

			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(page->tree), path, NULL, TRUE, 0.5, 0.5);
			state->allow_select = TRUE;
			gtk_tree_selection_select_path(selection, path);
			state->allow_select = FALSE;
//			gtk_widget_grab_focus(GTK_WIDGET(page->tree));
		} else {
			state->last_source_file = xdstrdup(filename_attr->value);
		}
	}

	return NON_INTERACTIVE;
}


int process_select_file_line_for_stack_0(int param, ClientState* state)
{
	_process_select_file_line_for_stack_0(param, state, 1);
}


int process_select_file_line_for_stack_0_no_check(int param, ClientState* state)
{
	_process_select_file_line_for_stack_0(param, state, 0);
}

static int _process_select_file_line_for_selected_stack(int param, ClientState* state, int check)
{
	/* Let's find the page! */
	/* - first we figure out stack frame 0, and the filename from it */
	xdebug_xml_node *message = state->message;
	xdebug_xml_attribute *level_attr, *filename_attr = NULL, *lineno_attr = NULL;
	dbgp_code_page *page;
	GtkWidget *code_notebook = GTK_WIDGET(gtk_builder_get_object(builder, "code_notebook"));
	GtkTreeSelection *selection;
	GtkTreePath      *path;
	xdebug_xml_node *stack_frame;

	stack_frame = state->message->child;
	while (stack_frame) {
		if (strcmp(stack_frame->tag, "stack") == 0) {
			level_attr = xdebug_xml_fetch_attribute(stack_frame, "level");

			if (level_attr && level_attr->value && strtol(level_attr->value, NULL, 10) == state->last_selected_stack_frame) {
				filename_attr = xdebug_xml_fetch_attribute(stack_frame, "filename");
				lineno_attr = xdebug_xml_fetch_attribute(stack_frame, "lineno");
			}
		}
		
		stack_frame = stack_frame->next;
	}

	if (!filename_attr) {
		return NON_INTERACTIVE;
	}

	/* Now we have the filename in attr->value */
	if (xdebug_hash_find(code_tabs_hash, filename_attr->value, strlen(filename_attr->value), (void*) &page)) {
		/* If we found it then we advance the action pointer 4 items so that we
		 * don't call an add_source for it, but only the first time. */
		if (check) {
			state->action_list_ptr += param;
		}

		/* then we select the page */
		gtk_notebook_set_current_page(GTK_NOTEBOOK(code_notebook), page->nr);
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(page->tree));
		path = gtk_tree_path_new_from_indices(atoi(lineno_attr->value) - 1, -1);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(page->tree), path, NULL, TRUE, 0.5, 0.5);
		state->allow_select = TRUE;
		gtk_tree_selection_select_path(selection, path);
		state->allow_select = FALSE;
//		gtk_widget_grab_focus(GTK_WIDGET(page->tree));
	} else {
		state->last_source_file = xdstrdup(filename_attr->value);
	}

	return NON_INTERACTIVE;
}


int process_select_file_line_for_selected_stack(int param, ClientState* state)
{
	_process_select_file_line_for_selected_stack(param, state, 1);
}


int process_select_file_line_for_selected_stack_no_check(int param, ClientState* state)
{
	_process_select_file_line_for_selected_stack(param, state, 0);
}

static void add_property(GtkTreeStore *store, GtkTreeIter *parent_iter, xdebug_xml_node *property)
{
	xdebug_xml_attribute *fullname_attr, *name_attr, *type_attr, *size_attr, *encoding_attr, *class_attr, *facet_attr;
	xdebug_xml_attribute *children_attr, *numchildren_attr, *page_attr;
	gchar *value, *type, *name;
	int new_len, max_children;
	GtkTreeIter iter;
	GConfEngine *conf;
	gint pages;
	GtkTreePath *path;
	gchar *path_string;
	int child_count, fetch_page;
	int offset = 0, hint = 0;

	conf = gconf_engine_get_default();
	max_children = gconf_engine_get_int(conf, "/apps/gtkdbgp/max_children", NULL);

	children_attr = xdebug_xml_fetch_attribute(property, "children");
	fullname_attr = xdebug_xml_fetch_attribute(property, "fullname");
	numchildren_attr = xdebug_xml_fetch_attribute(property, "numchildren");
	page_attr = xdebug_xml_fetch_attribute(property, "page");

	if (fullname_attr) {
		g_print( "\nRunning add_property for '%s'\n", fullname_attr->value);
	}

	if (children_attr && numchildren_attr && children_attr->value && strcmp(children_attr->value, "1") == 0) {
		gtk_tree_model_get(GTK_TREE_MODEL(store), parent_iter, VARVIEW_PAGES_FETCHED, &fetch_page, -1);
		gtk_tree_model_get(GTK_TREE_MODEL(store), parent_iter, VARVIEW_HIDDEN_HINT, &hint, -1);
		g_print( "Got fetched pages: %d\n", fetch_page);
		g_print( "Got hidden hint: %d\n", hint);

		pages = strtol(numchildren_attr->value, NULL, 10);
		pages = (pages + max_children - 1) / max_children;
		if ((fetch_page + 1) == pages) {
			g_print( "Setting hint to 0\n");
			gtk_tree_store_set(store, parent_iter,
				VARVIEW_HIDDEN_HINT, 0,
				-1);
		}

		g_print( "Setting page count to %d\n", pages);
		gtk_tree_store_set(store, parent_iter,
			VARVIEW_PAGE_COUNT,    pages,
			-1);

		if (page_attr) {
			g_print( "Setting pages fetched to %d\n", fetch_page + 1);
			gtk_tree_store_set(store, parent_iter,
				VARVIEW_PAGES_FETCHED, fetch_page + 1,
				-1);
		}

		offset = fetch_page * max_children;
	}
	if (!property->child && children_attr && children_attr->value && strcmp(children_attr->value, "1") == 0) {
		g_print( "Setting hint to %d\n", DBGPCLIENT_FETCH_MORE);
		gtk_tree_store_set(store, parent_iter,
			VARVIEW_HIDDEN_HINT, DBGPCLIENT_FETCH_MORE,
			-1);
		return;
	}

	property = property->child;
	child_count = 0;
	while (property) {
		if (strcmp(property->tag, "property") == 0) {
			child_count++;
			name_attr = xdebug_xml_fetch_attribute(property, "name");

			if (name_attr && name_attr->value && strcmp(name_attr->value, "CLASSNAME") == 0) {
				goto process_next;
			}
			type_attr = xdebug_xml_fetch_attribute(property, "type");
			size_attr = xdebug_xml_fetch_attribute(property, "size");
			encoding_attr = xdebug_xml_fetch_attribute(property, "encoding");
			fullname_attr = xdebug_xml_fetch_attribute(property, "fullname");

			if (property->text && property->text->text) {
				if (encoding_attr && strcmp(encoding_attr->value, "base64") == 0) {
					value = gnet_base64_decode(property->text->text, strlen(property->text->text), &new_len);
				} else {
					value = property->text->text;
				}
			} else {
				value = "";
			}

			name = name_attr->value ? name_attr->value : "*unknown*";
			if (name_attr->value) {
				facet_attr = xdebug_xml_fetch_attribute(property, "facet");
				if (facet_attr && facet_attr->value) {
					name = xdebug_sprintf("%s: %s", facet_attr->value, name);
				}
			}

			type = type_attr->value ? type_attr->value : "*unknown*";
			if (type_attr->value) {
				if (strcmp(type_attr->value, "object") == 0) {
					class_attr = xdebug_xml_fetch_attribute(property, "classname");
					if (class_attr && class_attr->value) {
						value = xdebug_sprintf("class = %s", class_attr->value);
					}
				}
			}

			gtk_tree_store_append(store, &iter, parent_iter);
			path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
			path_string = gtk_tree_path_to_string(path);
			gtk_tree_store_set(store, &iter,
				VARVIEW_NR_COLUMN,       name,
				VARVIEW_FUNCTION_COLUMN, type,
				VARVIEW_LOCATION_COLUMN, value,
				VARVIEW_FULLNAME,        fullname_attr ? fullname_attr->value : name_attr->value,
				VARVIEW_PATH_STRING,     path_string,
				-1);
			g_free(path_string);
			gtk_tree_path_free(path);

			add_property(store, &iter, property);
		}
process_next:
		property = property->next;
	}

	if (numchildren_attr && numchildren_attr->value &&
		((offset + child_count) < strtoul(numchildren_attr->value, NULL, 10))) {
		gtk_tree_store_set(store, parent_iter,
			VARVIEW_HIDDEN_HINT, DBGPCLIENT_FETCH_PAGES,
			-1);
	}
}

int process_update_context_vars_for_selected_stack(int param, ClientState* state)
{
	GtkWidget *var_view;
	GtkTreeStore *store;
	xdebug_xml_node *property;

 	var_view = GTK_WIDGET(gtk_builder_get_object(builder, "var_view"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(var_view)));

	gtk_tree_store_clear(store);

	property = state->message;

	add_property(store, NULL, property);

	return NON_INTERACTIVE;
}


int process_update_stack(int param, ClientState* state)
{
	GtkWidget *stack_view;
	GtkListStore *store;
	GtkTreeIter   iter;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	xdebug_xml_node *stack_frame;
	xdebug_xml_attribute *where_attr, *level_attr, *filename_attr, *lineno_attr;

 	stack_view = GTK_WIDGET(gtk_builder_get_object(builder, "stack_view"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(stack_view)));
	state->nr_of_stack_frames = 0;

	gtk_list_store_clear(store);

	stack_frame = state->message->child;
	while (stack_frame) {
		if (strcmp(stack_frame->tag, "stack") == 0) {
			char *tmp_loc;

			where_attr = xdebug_xml_fetch_attribute(stack_frame, "where");
			level_attr = xdebug_xml_fetch_attribute(stack_frame, "level");
			filename_attr = xdebug_xml_fetch_attribute(stack_frame, "filename");
			lineno_attr = xdebug_xml_fetch_attribute(stack_frame, "lineno");

			tmp_loc = xdebug_sprintf("%s:%s", filename_attr->value, lineno_attr->value);

			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter,
				STACK_NR_COLUMN,       level_attr->value,
				STACK_FUNCTION_COLUMN, where_attr->value,
				STACK_LOCATION_COLUMN, tmp_loc,
				-1);

			xdfree(tmp_loc);
			state->nr_of_stack_frames++;
		}
		
		stack_frame = stack_frame->next;
	}

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(stack_view));
	path = gtk_tree_path_new_from_indices(0, -1);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(stack_view), path, NULL, TRUE, 0.5, 0.5);
	state->auto_stack_select = TRUE;
	gtk_tree_selection_select_path(selection, path);
	state->last_selected_stack_frame = 0;
	state->auto_stack_select = FALSE;

	return NON_INTERACTIVE;
}



action_item initial_action_list[] = {
	{ process_mark_buttons_inactive, 0 },
	{ do_source_get, 0 },
	{ process_add_source_file, 0 },
	{ do_step_into, 0 },
	{ do_stack_get, 0 },
	{ process_update_stack, 0 },
	{ process_select_file_line_for_stack_0, 4 },
	{ do_source_get, 0 },
	{ process_add_source_file, 0 },
	{ do_stack_get, 0 },
	{ process_select_file_line_for_stack_0_no_check, 0 },
	{ do_context_get, 0 },
	{ process_update_context_vars_for_selected_stack, 0 },
	{ process_jump, JMP_IF_BREAK_ON_ERROR_FEATURE | 4 },
	{ do_set_error_breakpoint, DBGP_PHP_NOTICE },
	{ do_set_error_breakpoint, DBGP_PHP_WARNING },
	{ do_set_error_breakpoint, DBGP_PHP_ERROR },
	{ do_set_error_breakpoint, DBGP_PHP_STRICT },
	{ do_set_feature, FEATURE_MAX_CHILDREN },
	{ do_set_feature, FEATURE_MAX_DEPTH },
	{ do_set_feature, FEATURE_MAX_DATA },
	{ process_mark_buttons_active, 0 },
	{ NULL, 0 }
};

action_item break_action_list[] = {
	{ process_mark_buttons_inactive, 0 },
	{ do_stack_get, 0 },
	{ process_update_stack, 0 },
	{ process_select_file_line_for_stack_0, 4 },
	{ do_source_get, 0 },
	{ process_add_source_file, 0 },
	{ do_stack_get, 0 },
	{ process_select_file_line_for_stack_0_no_check, 0 },
	{ do_context_get, 0 },
	{ process_update_context_vars_for_selected_stack, 0 },
	{ process_mark_buttons_active, 0 },
	{ NULL, 0 }
};

action_item stopped_action_list[] = {
	{ process_mark_buttons_inactive, 0 },
	{ do_run, 0 },
	{ NULL, 0 }
};

action_item select_stack_action_list[] = {
	{ process_mark_buttons_inactive, 0 },
	{ process_select_file_line_for_selected_stack, 4 },
	{ do_source_get, 0 },
	{ process_add_source_file, 0 },
	{ do_stack_get, 0 },
	{ process_select_file_line_for_selected_stack_no_check, 0 },
	{ do_context_get, 0 },
	{ process_update_context_vars_for_selected_stack, 0 },
	{ process_mark_buttons_active, 0 },
	{ NULL, 0 }
};

action_item fetch_property_action_list[] = {
	{ process_mark_buttons_inactive, 0 },
	{ process_fetched_property, 0 },
	{ process_mark_buttons_active, 0 },
	{ NULL, 0 }
};

static void record_open_rows(GtkTreeView *tree_view, GtkTreePath *path, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *var_name;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, VARVIEW_FULLNAME, &var_name, -1);

	printf("PATH===%s\n", var_name);
}

static process_a_button(gchar *command, GtkToolButton *toolbutton, gpointer user_data)
{
	GIOChannel *iochannel;
	GtkWidget *statusbar;
	GtkWidget *stack_view;
	GtkTreeModel *store;

	stack_view = GTK_WIDGET(gtk_builder_get_object(builder, "stack_view"));
	store = gtk_tree_view_get_model(GTK_TREE_VIEW(stack_view));

	printf("SHOWING PATHS\n");
	{
		GtkWidget *var_view;
		int i;

		var_view = GTK_WIDGET(gtk_builder_get_object(builder, "var_view"));
		printf("CS INFO\n- nr of frames: %d\n", client_state->nr_of_stack_frames);
		printf("- current size: %d\n", client_state->stack_info_size);
		if (client_state->nr_of_stack_frames > client_state->stack_info_size) {
			client_state->stack_info = realloc(client_state->stack_info, sizeof(stack_element_info*) * (client_state->nr_of_stack_frames + 4));
			for (i = client_state->stack_info_size; i < client_state->nr_of_stack_frames + 4; i++) {
				printf("Setting [%d] to NULL\n", i);
				client_state->stack_info[i] = NULL;
			}
			client_state->stack_info_size = client_state->nr_of_stack_frames + 4;
			printf("- new size: %d\n", client_state->stack_info_size);
		}

		printf("- saving for [%d]\n", client_state->nr_of_stack_frames - 1);
		gtk_tree_view_map_expanded_rows(GTK_TREE_VIEW(var_view), record_open_rows, (gpointer) client_state->stack_info[client_state->nr_of_stack_frames - 1]);
	}

	if (client_state && client_state->buttons_active) {
		update_statusbar("Processing...");
		client_state->command = xdebug_sprintf("%s -i %d", command, get_next_id(client_state));
		client_state->watch_flags |= G_IO_OUT;
		iochannel = gnet_tcp_socket_get_io_channel(client_state->socket);
		g_source_remove(client_state->watch);
		client_state->watch = g_io_add_watch(iochannel, client_state->watch_flags, async_client_iofunc, client_state);

		statusbar = GTK_WIDGET(gtk_builder_get_object(builder, "last_message_label"));
		gtk_label_set(GTK_LABEL(statusbar), "");
	}
}

void process_continue_button(GtkToolButton *toolbutton, gpointer user_data)
{
	process_a_button("run", toolbutton, user_data);
}


void process_detach_button(GtkToolButton *toolbutton, gpointer user_data)
{
	process_a_button("detach", toolbutton, user_data);
}


void process_kill_button(GtkToolButton *toolbutton, gpointer user_data)
{
	process_a_button("stop", toolbutton, user_data);
}


void process_step_into_button(GtkToolButton *toolbutton, gpointer user_data)
{
	process_a_button("step_into", toolbutton, user_data);
}


void process_step_out_button(GtkToolButton *toolbutton, gpointer user_data)
{
	process_a_button("step_out", toolbutton, user_data);
}


void process_step_over_button(GtkToolButton *toolbutton, gpointer user_data)
{
	process_a_button("step_over", toolbutton, user_data);
}





int get_next_id(ClientState * client_state)
{
	client_state->last_tid++;
	return client_state->last_tid;
}

gboolean code_page_selection_function (GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, gpointer userdata)
{
	if (client_state->allow_select) {
		return TRUE;
	} else {
		return FALSE;
	}
}

gboolean stack_selection_function(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, gpointer userdata)
{
	GtkTreeIter iter;
	GIOChannel *iochannel;
	gchar *name;

	if (!path_currently_selected && !client_state->auto_stack_select && gtk_tree_model_get_iter(model, &iter, path)) {
		gtk_tree_model_get(model, &iter, STACK_NR_COLUMN, &name, -1);
		if (strtol(name, NULL, 10) != client_state->last_selected_stack_frame) {
			client_state->last_selected_stack_frame = strtol(name, NULL, 10);

			client_state->server_state = SERVER_STATE_SELECT_STACK;
			client_state->action_list_ptr = select_stack_action_list;

			do_stack_get(0, client_state);

			client_state->watch_flags |= G_IO_OUT;
			iochannel = gnet_tcp_socket_get_io_channel(client_state->socket);
			g_source_remove(client_state->watch);
			client_state->watch = g_io_add_watch(iochannel, client_state->watch_flags, async_client_iofunc, client_state);
		}
		g_free(name);
	}
	return TRUE; /* allow selection state to change */
}

gboolean varview_selection_function(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, gpointer userdata)
{
	GtkTreeIter iter;
	GIOChannel *iochannel;
	gchar *fullname;
	gint   hint;
	gint   fetch_page;

	if (!path_currently_selected && gtk_tree_model_get_iter(model, &iter, path)) {
		gtk_tree_model_get(model, &iter, VARVIEW_FULLNAME, &fullname, -1);
		gtk_tree_model_get(model, &iter, VARVIEW_HIDDEN_HINT, &hint, -1);
		gtk_tree_model_get(model, &iter, VARVIEW_PAGES_FETCHED, &fetch_page, -1);

		g_print( "Need to fetch '%s'? The hint is '%d'\n", fullname, hint);
		if (hint == DBGPCLIENT_FETCH_MORE) {
			gtk_tree_store_set(GTK_TREE_STORE(model), &iter, VARVIEW_HIDDEN_HINT, 0, -1);
			g_print( "Setting hint to 0\n");
			client_state->server_state = SERVER_STATE_FETCH_PROPERTY;
			client_state->action_list_ptr = fetch_property_action_list;
			client_state->property = xdstrdup(fullname);
			client_state->path_string = gtk_tree_path_to_string(path);

			do_property_get(0, client_state);

			client_state->watch_flags |= G_IO_OUT;
			iochannel = gnet_tcp_socket_get_io_channel(client_state->socket);
			g_source_remove(client_state->watch);
			client_state->watch = g_io_add_watch(iochannel, client_state->watch_flags, async_client_iofunc, client_state);
		}
		if (hint == DBGPCLIENT_FETCH_PAGES) {
			gtk_tree_store_set(GTK_TREE_STORE(model), &iter, VARVIEW_HIDDEN_HINT, 0, -1);
			g_print( "Setting hint to 0\n");
			client_state->server_state = SERVER_STATE_FETCH_PROPERTY;
			client_state->action_list_ptr = fetch_property_action_list;
			client_state->property = xdstrdup(fullname);
			client_state->path_string = gtk_tree_path_to_string(path);

			do_property_get(fetch_page, client_state);

			client_state->watch_flags |= G_IO_OUT;
			iochannel = gnet_tcp_socket_get_io_channel(client_state->socket);
			g_source_remove(client_state->watch);
			client_state->watch = g_io_add_watch(iochannel, client_state->watch_flags, async_client_iofunc, client_state);
		}
		g_free(fullname);
	}
	return TRUE; /* allow selection state to change */
}

void add_source_file(gchar* filename, gchar *source)
{
	GtkWidget *treeview3;
	GtkWidget *label1, *scrolledwindow, *eventbox1;
	GtkWidget *code_notebook = GTK_WIDGET(gtk_builder_get_object(builder, "code_notebook"));
	GtkTooltips *tooltips;
	GtkTreeViewColumn *column1, *column2, *column3, *column4;
	GtkTreeSelection *selection;
	GtkListStore *store;
	GtkTreeIter   iter;
	GtkCellRenderer *r1, *r2, *r3, *r4;
	gint new_len, i;
	gchar *unencoded, *sanitized_label;
	xdebug_arg *lines;
	dbgp_code_page *page;
	GConfEngine *conf;
	gchar *code_font;

	if (xdebug_hash_find(code_tabs_hash, filename, strlen(filename), (void*) &page))
	{
		return;
	}

	/* Fetch the font */
	conf = gconf_engine_get_default();
	code_font = gconf_engine_get_string(conf, "/apps/gtkdbgp/font/code", NULL);

	/* Setup the store */
	store = gtk_list_store_new (SOURCE_N_COLUMNS, G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);

	/* Add data */
	unencoded = gnet_base64_decode(source, strlen(source), &new_len);
	lines = (xdebug_arg*) xdmalloc(sizeof(xdebug_arg));
	xdebug_arg_init(lines);
	xdebug_explode("\n", unencoded, lines, -1);
	for (i = 0; i < lines->c; i++) {
		char *lineno = xdebug_sprintf("%d", i + 1);

		gtk_list_store_append (store, &iter);  /* Acquire an iterator */
		gtk_list_store_set (store, &iter,
			SOURCE_ST_COLUMN, NULL, 
			SOURCE_LINENO_COLUMN, lineno,
			SOURCE_LINE_COLUMN, lines->args[i],
			-1);
		xdfree(lineno);
	}
	xdebug_arg_dtor(lines);

	/* Make the tree view thingy */
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_name(scrolledwindow, "sc1");
	gtk_widget_show (scrolledwindow);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);

	treeview3 = gtk_tree_view_new();
	gtk_widget_set_name (treeview3, "treeview3");
	gtk_widget_show (treeview3);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview3));
//	gtk_tree_selection_set_select_function(selection, code_page_selection_function, NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolledwindow), treeview3);

	page = (dbgp_code_page*) malloc(sizeof (dbgp_code_page));
	page->nr = gtk_notebook_append_page(GTK_NOTEBOOK(code_notebook), scrolledwindow, NULL);
	page->tree = treeview3;
	gtk_notebook_set_current_page(GTK_NOTEBOOK(code_notebook), page->nr);

	tooltips = gtk_tooltips_new();
	eventbox1 = gtk_event_box_new();
	gtk_widget_set_name(eventbox1, "eventbox1");
	gtk_widget_show(eventbox1);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK (code_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (code_notebook), page->nr), eventbox1);
	gtk_tooltips_set_tip(tooltips, eventbox1, filename, NULL);

	sanitized_label = strrchr(filename, '/') + 1;
	label1 = gtk_label_new (sanitized_label);
	gtk_widget_set_name (label1, "label1");
	gtk_widget_show (label1);
	gtk_container_add(GTK_CONTAINER(eventbox1), label1);

	/* Do the columns */
	r2 = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview3), -1, "", r2, "pixbuf", SOURCE_ST_COLUMN, NULL);

	r3 = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview3), -1, "#", r3, "text", SOURCE_LINENO_COLUMN, NULL);

	r4 = gtk_cell_renderer_text_new();
	g_object_set(r4, "font-desc", pango_font_description_from_string (code_font), NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview3), -1, "Code", r4, "text", SOURCE_LINE_COLUMN, NULL);

	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview3), GTK_TREE_MODEL(store));

	xdebug_hash_add(code_tabs_hash, filename, strlen(filename), (void *) page);
}

char* process_init(xdebug_xml_node *init)
{
	xdebug_xml_attribute *attr;

	attr = xdebug_xml_fetch_attribute(init, "fileuri");
	printf("fileuri = %s\n", attr->value);
	if (attr && attr->value) {
		return xdstrdup(attr->value);
	}
	return NULL;
}

void process_stack_get(xdebug_xml_node *cg)
{
}

void update_statusbar(gchar* text)
{
	GtkWidget *statusbar = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));

	gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "test"), text);
}

int process_state_input(ClientState *client_state)
{
	xdebug_xml_node      *message = client_state->message;
	xdebug_xml_attribute *attr;

	if (strcmp(message->tag, "init") == 0) {
		/* We need to do initialization */
		attr = xdebug_xml_fetch_attribute(message, "fileuri");
		if (attr && attr->value) {
			client_state->last_source_file = xdstrdup(attr->value);
		}
		client_state->server_state = SERVER_STATE_INITIAL;
		client_state->action_list_ptr = initial_action_list;
	}
	if (strcmp(message->tag, "response") == 0) {
		attr = xdebug_xml_fetch_attribute(message, "status");
		if (attr && attr->value && strcmp(attr->value, "break") == 0) {
			client_state->server_state = SERVER_STATE_BREAK;
			client_state->action_list_ptr = break_action_list;
		}
		attr = xdebug_xml_fetch_attribute(message, "reason");
		if (attr && attr->value && strcmp(attr->value, "exception") == 0) {
			gchar *error_message;
			xdebug_xml_attribute *level, *code;
			GtkWidget *statusbar;
			
			statusbar = GTK_WIDGET(gtk_builder_get_object(builder, "last_message_label"));
			level = xdebug_xml_fetch_attribute(message->child, "exception");
			code = xdebug_xml_fetch_attribute(message->child, "code");

			error_message = xdebug_sprintf("<span foreground=\"red\" weight=\"bold\">%s:</span> %s (%s)", level->value, message->child->text->text, code->value);
			gtk_label_set_markup(GTK_LABEL(statusbar), error_message);
			xdfree(error_message);
		}
	}
	if (strcmp(message->tag, "response") == 0) {
		attr = xdebug_xml_fetch_attribute(message, "status");
		if (attr && attr->value && strcmp(attr->value, "stopped") == 0) {
			client_state->server_state = SERVER_STATE_STOPPED;
			client_state->action_list_ptr = stopped_action_list;
		}
	}
	printf("STATE: %d\n", client_state->server_state );
}

int process_state_init(ClientState *client_state)
{
	action_item action;

	/* Take action from the list */
	action = *client_state->action_list_ptr;

	while (action.func && action.func(action.param, client_state) == NON_INTERACTIVE)
	{
		/* Increase action pointer */
		client_state->action_list_ptr++;
		/* Get a new action */
		action = *client_state->action_list_ptr;
	}
	if (!action.func) {
		client_state->server_state = SERVER_STATE_INPUT;
	} else {
		/* Increase action pointer */
		client_state->action_list_ptr++;
	}
}


int process_state_break(ClientState *client_state)
{
}


int process_state_stopped(ClientState *client_state)
{
}



int process_request(ClientState *client_state, int bcount)
{
	char *src_file, *new_cmd = NULL;
	gchar* buffer = client_state->buffer;

	client_state->message = parse_message(buffer, bcount);

	if (client_state->server_state == SERVER_STATE_INPUT) {
		process_state_input(client_state);
	}

	switch (client_state->server_state) {
		case SERVER_STATE_INITIAL:
		case SERVER_STATE_BREAK:
		case SERVER_STATE_STOPPED:
		case SERVER_STATE_SELECT_STACK:
		case SERVER_STATE_FETCH_PROPERTY:
			process_state_init(client_state);
			break;
	}

	if (client_state->command) {
		return 1;
	} else {
		return 0;
	}
}


int start_server(gint port)
{
	GTcpSocket* server;
	GInetAddr*  addr;
	gchar*      name;
	GMainLoop*  main_loop;

	gnet_init ();

	/* Create the server */
	server = gnet_tcp_socket_server_new_with_port (port);
	if (server == NULL)
	{
		fprintf (stderr, "Could not create server on port %d\n", port);
		exit (EXIT_FAILURE);
	}

	async_server = server;
	signal (SIGINT, async_sig_int);

	/* Print the address */
	addr = gnet_tcp_socket_get_local_inetaddr(server);
	g_assert (addr);
	name = gnet_inetaddr_get_canonical_name (addr);
	g_assert (name);
	port = gnet_inetaddr_get_port (addr);
	g_print ("Async echoserver running on %s:%d\n", name, port);
	update_statusbar("Waiting for connection");
	gnet_inetaddr_delete (addr);
	g_free (name);

	code_tabs_hash = xdebug_hash_alloc(32, NULL);

	/* Wait asyncy for incoming clients */
	gnet_tcp_socket_server_accept_async (server, async_accept, NULL);

	return 0;
}

static void clientstate_delete (ClientState* state)
{
	g_source_remove(state->watch);
	gnet_tcp_socket_delete(state->socket);
	g_free(state->name);
	g_free(state);
	state = NULL;
}

static void async_accept (GTcpSocket* server, GTcpSocket* client, gpointer data)
{
	if (client)
	{
		GInetAddr*  addr;
		gchar*      name;
		gint	  port;
		GIOChannel* client_iochannel;

		/* Stop more connections */
		/* gnet_tcp_socket_server_accept_async_cancel (server); */

		/* Print the address */
		addr = gnet_tcp_socket_get_local_inetaddr(client);
		g_assert (addr);
		name = gnet_inetaddr_get_canonical_name (addr);
		g_assert (name);
		port = gnet_inetaddr_get_port (addr);
		gnet_inetaddr_delete (addr);

		client_iochannel = gnet_tcp_socket_get_io_channel (client);
		g_assert (client_iochannel != NULL);

		client_state = g_new0(ClientState, 1);
		client_state->socket = client;
		client_state->name = name;
		client_state->port = port;
		client_state->watch_flags = G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
		client_state->watch = g_io_add_watch (client_iochannel, client_state->watch_flags, async_client_iofunc, client_state);
		client_state->command = NULL;
		client_state->last_tid = 1;
		client_state->server_state = SERVER_STATE_INPUT;

		client_state->first_packet = TRUE;
		client_state->more_coming = FALSE;
		client_state->expected_length = 0;

		client_state->buttons_active = FALSE;
		client_state->auto_stack_select = FALSE;

		memset(&client_state->last_iter, 0, sizeof(GtkTreeIter));
		client_state->last_store = NULL;


		client_state->stack_info_size = 0;
		client_state->stack_info = NULL;
		client_state->nr_of_stack_frames = 0;

		g_print ("Accepted connection from %s:%d\n", name, port);
		update_statusbar( "Waiting for input." );
	}
	else
	{
		fprintf (stderr, "Server error\n");
		exit (EXIT_FAILURE);
	}
}


/*

  Client IO callback.  Called for errors, input, or output.  When
  there is input, we reset the watch with the output flag (if we
  haven't already).  When there is no more data, the watch is reset
  without the output flag.
  
 */
gboolean async_client_iofunc (GIOChannel* iochannel, GIOCondition condition, gpointer data)
{
	ClientState* client_state = (ClientState*) data;
	gboolean rv = TRUE;

	g_assert (client_state != NULL);

	/* Check for socket error */
	if (condition & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
	{
		fprintf (stderr, "Client socket error (%s:%d)\n", client_state->name, client_state->port);
		goto error;
	}

	/* Check for data to be read (or if the socket was closed) */
	if (condition & G_IO_IN)
	{
		GIOError error;
		guint bytes_read;
		int null_starts = 0;
		char buffer[102400];
		guint expected_length = 0;

		/* Read the data into our buffer */
		memset(buffer, 0, sizeof(buffer));
		error = g_io_channel_read(iochannel, buffer, sizeof(buffer), (gsize *) &bytes_read);

		/* Check for socket error */
		if (error != G_IO_ERROR_NONE)
		{
			fprintf (stderr, "Client read error (%s:%d): %d\n", client_state->name, client_state->port, error);
			goto error;
		}

		/* Check if we read 0 bytes, which means the connection is
		closed */
		else if (bytes_read == 0)
		{
			g_print ("Connection from %s:%d closed\n", client_state->name, client_state->port);
			update_statusbar("Connection closed");
			goto error;
		}

		/* Otherwise, we read something */
		else
		{
			guint old_watch_flags;

			g_assert (bytes_read > 0);

			/* read the packet info and stuff */
			if (client_state->first_packet) {
				client_state->first_packet = FALSE;
				null_starts = strlen(buffer);
				memset(client_state->buffer, 0, sizeof(client_state->buffer));
				memcpy(client_state->buffer, buffer + null_starts + 1, bytes_read - null_starts - 1);
				client_state->current_pos = bytes_read - null_starts - 1;

				expected_length = strtol(buffer, NULL, 10);

				client_state->expected_length = expected_length;
				if (bytes_read - null_starts - 1 < expected_length) {
					client_state->more_coming = TRUE;
				} else {
					client_state->more_coming = FALSE;
				}
			} else {
				memcpy(client_state->buffer + client_state->current_pos, buffer, bytes_read);
				client_state->current_pos += bytes_read;
				if (client_state->current_pos >= client_state->expected_length) {
					client_state->more_coming = FALSE;
				}
			}

			old_watch_flags = client_state->watch_flags;

			if (!client_state->more_coming) {
				if (process_request(client_state, client_state->expected_length)) {
					/* Add an add watch */
					client_state->watch_flags |= G_IO_OUT;
				}
				client_state->first_packet = TRUE;
				client_state->more_coming = FALSE;
			}

			/* Update watch flags if they changed */
			if (old_watch_flags != client_state->watch_flags)
			{
				g_source_remove (client_state->watch);
				client_state->watch = g_io_add_watch(iochannel, client_state->watch_flags, async_client_iofunc, client_state);
				rv = FALSE;
			}

		}

	}

	if (condition & G_IO_OUT)
	{
		GIOError error;
		guint bytes_written;

		/* Write the data out */
		error = g_io_channel_write(iochannel, client_state->command, strlen(client_state->command) + 1 /*"source -i 1 -f file:///home/httpd/html/test/xdebug/bug111.php", 62*/, (gsize *) &bytes_written);
		xdfree(client_state->command);
		client_state->command = NULL;

		if (error != G_IO_ERROR_NONE) 
		{
			fprintf (stderr, "Client write error (%s:%d): %d\n", 
			client_state->name, client_state->port, error);
			goto error;
		}

		else if (bytes_written > 0)
		{
			guint old_watch_flags;

			old_watch_flags = client_state->watch_flags;

			/* Remove OUT watch if done */
			client_state->watch_flags &= ~G_IO_OUT;

			/* Add IN watch */
			client_state->watch_flags |= G_IO_IN;

			/* Update watch flags if they changed */
			if (old_watch_flags != client_state->watch_flags)
			{
				g_source_remove (client_state->watch);
				client_state->watch = g_io_add_watch(iochannel, client_state->watch_flags, async_client_iofunc, client_state);
				rv = FALSE;
			}
		}
	}
	return rv;

error:
	clientstate_delete(client_state);

	{
		GtkWidget *var_view, *stack_view;
		GtkTreeStore *store;

		stack_view = GTK_WIDGET(gtk_builder_get_object(builder, "stack_view"));
		store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(stack_view)));
		gtk_list_store_clear(GTK_LIST_STORE(store));

		var_view = GTK_WIDGET(gtk_builder_get_object(builder, "var_view"));
		store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(var_view)));
		gtk_tree_store_clear(store);
	}
	return FALSE;
}


static void async_sig_int(int signum)
{
	gnet_tcp_socket_delete(async_server);
	exit(EXIT_FAILURE);
}
