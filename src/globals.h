#include "xdebug_xml.h"
#include <gnet.h>

typedef struct _ClientState ClientState;

typedef struct action_item
{
	int (*func)(int, struct _ClientState*);
	int param;
} action_item;

typedef struct dbgp_code_page
{
	int nr;
	GtkWidget *tree;
} dbgp_code_page;

struct _ClientState
{
	GTcpSocket* socket;
	gchar* name;
	guint watch_flags;
	gint port;
	guint watch;

	gchar buffer[102400];
	gint first_packet;
	gint more_coming;
	guint expected_length;
	guint current_pos;

	gchar *command;
	int last_tid;
	int server_state; // INITIAL, BREAK, STOPPED
	action_item *action_list_ptr;

	xdebug_xml_node* message;
	gchar *last_source_file;
	char last_selected_stack_frame;

	int allow_select;
	int buttons_active;
	int auto_stack_select;
};


enum
{
	STACK_NR_COLUMN = 0,
	STACK_FUNCTION_COLUMN,
	STACK_LOCATION_COLUMN,
	STACK_N_COLUMNS
};

ClientState* client_state;

int get_next_id(ClientState * client_state);
gboolean stack_selection_function(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, gpointer userdata);

void process_continue_button(GtkToolButton *toolbutton, gpointer user_data);
void process_step_into_button(GtkToolButton *toolbutton, gpointer user_data);
void process_step_out_button(GtkToolButton *toolbutton, gpointer user_data);
void process_step_over_button(GtkToolButton *toolbutton, gpointer user_data);

