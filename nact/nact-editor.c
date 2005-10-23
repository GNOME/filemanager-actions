/* Nautilus Actions configuration tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
 *	 Rodrigo Moya (rodrigo@gnome-db.org)
 *  Frederic Ruaudel (grumz@grumz.net)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtktogglebutton.h>
#include <glade/glade-xml.h>
#include "nact-editor.h"
#include "nact-utils.h"
#include "nact-prefs.h"

enum {
	ICON_STOCK_COLUMN = 0,
	ICON_LABEL_COLUMN,
	ICON_N_COLUMN
};

enum {
	SCHEMES_CHECKBOX_COLUMN = 0,
	SCHEMES_KEYWORD_COLUMN,
	SCHEMES_DESC_COLUMN,
	SCHEMES_N_COLUMN
};

static gchar *
strip_underscore (const gchar *text)
{
	/* Code from gtk-demo */
	gchar *p, *q;
	gchar *result;

	result = g_strdup (text);
	p = q = result;
	while (*p) 
	{
		if (*p != '_')
		{
			*q = *p;
			q++;
		}
		p++;
	}
	*q = '\0';

	return result;
}

void
field_changed_cb (GObject *object, gpointer user_data)
{
	GtkWidget* editor = nact_get_glade_widget_from ("EditActionDialog", GLADE_EDIT_DIALOG_WIDGET);
	GtkWidget* menu_label = nact_get_glade_widget_from ("MenuLabelEntry", GLADE_EDIT_DIALOG_WIDGET);
	const gchar *label = gtk_entry_get_text (GTK_ENTRY (menu_label));

	if (label && strlen (label) > 0)
		gtk_dialog_set_response_sensitive (GTK_DIALOG (editor), GTK_RESPONSE_OK, TRUE);
	else
		gtk_dialog_set_response_sensitive (GTK_DIALOG (editor), GTK_RESPONSE_OK, FALSE);
}

void icon_browse_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	gchar* last_dir;
	gchar* filename;
	GtkWidget* filechooser = nact_get_glade_widget_from ("FileChooserDialog", GLADE_FILECHOOSER_DIALOG_WIDGET);
	GtkWidget* combo = nact_get_glade_widget_from ("MenuIconComboBoxEntry", GLADE_EDIT_DIALOG_WIDGET);
	gboolean set_current_location = FALSE;

	filename = (gchar*)gtk_entry_get_text (GTK_ENTRY (GTK_BIN (combo)->child));
	if (filename != NULL && strlen (filename) > 0)
	{
		set_current_location = gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (filechooser), filename);
	}
	
	if (!set_current_location)
	{
		last_dir = nact_prefs_get_icon_last_browsed_dir ();
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (filechooser), last_dir);
		g_free (last_dir);
	}

	switch (gtk_dialog_run (GTK_DIALOG (filechooser))) 
	{
		case GTK_RESPONSE_OK :
			last_dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (filechooser));
			nact_prefs_set_icon_last_browsed_dir (last_dir);
			g_free (last_dir);

			filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filechooser));
			gtk_entry_set_text (GTK_ENTRY (GTK_BIN (combo)->child), filename);
			g_free (filename);
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_DELETE_EVENT:
			gtk_widget_hide (filechooser);
	}		
}


void path_browse_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	gchar* last_dir;
	gchar* filename;
	GtkWidget* filechooser = nact_get_glade_widget_from ("FileChooserDialog", GLADE_FILECHOOSER_DIALOG_WIDGET);
	GtkWidget* entry = nact_get_glade_widget_from ("CommandPathEntry", GLADE_EDIT_DIALOG_WIDGET);
	gboolean set_current_location = FALSE;

	filename = (gchar*)gtk_entry_get_text (GTK_ENTRY (entry));
	if (filename != NULL && strlen (filename) > 0)
	{
		set_current_location = gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (filechooser), filename);
	}
	
	if (!set_current_location)
	{
		last_dir = nact_prefs_get_path_last_browsed_dir ();
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (filechooser), last_dir);
		g_free (last_dir);
	}

	switch (gtk_dialog_run (GTK_DIALOG (filechooser))) 
	{
		case GTK_RESPONSE_OK :
			last_dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (filechooser));
			nact_prefs_set_path_last_browsed_dir (last_dir);
			g_free (last_dir);

			filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filechooser));
			gtk_entry_set_text (GTK_ENTRY (entry), filename);
			g_free (filename);
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_DELETE_EVENT:
			gtk_widget_hide (filechooser);
	}		
}

void
legend_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkWidget* editor = nact_get_glade_widget_from ("EditActionDialog", GLADE_EDIT_DIALOG_WIDGET);
	GladeXML *gui;
	GtkWidget *legend_dialog;

	gui = nact_get_glade_xml_object ("LegendDialog");
	if (!gui)
		return;

	legend_dialog = nact_get_glade_widget_from ("LegendDialog", GLADE_LEGEND_DIALOG_WIDGET);
	gtk_window_set_transient_for (GTK_WINDOW (legend_dialog), GTK_WINDOW (editor));

	gtk_dialog_run (GTK_DIALOG (legend_dialog));

	gtk_widget_hide (legend_dialog);
	g_object_unref (gui);
}

static GtkTreeModel* create_stock_icon_model (void)
{
	GSList* stock_list = NULL;
	GSList* iter;
	GtkListStore* model;
	GtkTreeIter row;
	GtkWidget* window = nact_get_glade_widget_from ("EditActionDialog", GLADE_EDIT_DIALOG_WIDGET);
	GdkPixbuf* icon = NULL;
	GtkStockItem stock_item;
	gchar* label;

	model = gtk_list_store_new (ICON_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING);
	
	gtk_list_store_append (model, &row);
	
	gtk_list_store_set (model, &row, ICON_STOCK_COLUMN, "", ICON_LABEL_COLUMN, _("None"), -1);
	stock_list = gtk_stock_list_ids ();

	for (iter = stock_list; iter; iter = iter->next)
	{
		if (gtk_stock_lookup ((gchar*)iter->data, &stock_item))
		{
			icon = gtk_widget_render_icon (window, (gchar*)iter->data, GTK_ICON_SIZE_MENU, NULL);
			gtk_list_store_append (model, &row);
			label = strip_underscore (stock_item.label);
			gtk_list_store_set (model, &row, ICON_STOCK_COLUMN, (gchar*)iter->data, ICON_LABEL_COLUMN, label, -1);
			g_free (label);
		}
	}

	g_slist_foreach (stock_list, (GFunc)g_free, NULL);
	g_slist_free (stock_list);

	return GTK_TREE_MODEL (model);
}

static void preview_icon_changed_cb (GtkEntry* icon_entry, gpointer user_data)
{
	GtkWidget* image = nact_get_glade_widget_from ("IconImage", GLADE_EDIT_DIALOG_WIDGET);
	const gchar* icon_name = gtk_entry_get_text (icon_entry);
	GtkStockItem stock_item;
	GdkPixbuf* icon = NULL;

	if (icon_name && strlen (icon_name) > 0)
	{
		if (gtk_stock_lookup (icon_name, &stock_item))
		{
			gtk_image_set_from_stock (GTK_IMAGE (image), icon_name, GTK_ICON_SIZE_MENU);
		}
		else if (g_file_test (icon_name, G_FILE_TEST_EXISTS) && 
					g_file_test (icon_name, G_FILE_TEST_IS_REGULAR))
		{
			gint width;
			gint height;
			GError* error = NULL;
			
			gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);
			icon = gdk_pixbuf_new_from_file_at_size (icon_name, width, height, &error);
			if (error)
			{
				icon = NULL;
			}
			gtk_image_set_from_pixbuf (GTK_IMAGE (image), icon);
		}

		gtk_widget_show (image);
	}
	else
	{
		gtk_widget_hide (image);
	}
}

static void fill_menu_icon_combo_list_of (GtkComboBoxEntry* combo)
{
	GtkCellRenderer *cell_renderer_pix;
	GtkCellRenderer *cell_renderer_text;
	
	gtk_combo_box_set_model (GTK_COMBO_BOX (combo), create_stock_icon_model ());
	gtk_combo_box_entry_set_text_column (combo, ICON_STOCK_COLUMN);
	gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));

	cell_renderer_pix = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
		 cell_renderer_pix,
		 FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), cell_renderer_pix,
		"stock-id", ICON_STOCK_COLUMN);

	cell_renderer_text = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
		 cell_renderer_text,
		 TRUE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), cell_renderer_text,
		"text", ICON_LABEL_COLUMN);

	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
}

static void scheme_selection_toggled_cb (GtkCellRendererToggle *cell_renderer,
													  gchar *path_str,
													  gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreePath* path;
	gboolean toggle_state;
	GtkTreeModel* model = GTK_TREE_MODEL (user_data);

	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, SCHEMES_CHECKBOX_COLUMN, &toggle_state, -1);

	gtk_list_store_set (GTK_LIST_STORE (model), &iter, SCHEMES_CHECKBOX_COLUMN, !toggle_state, -1);

	//--> Notice edition change
	field_changed_cb (G_OBJECT (cell_renderer), NULL);

	gtk_tree_path_free (path);
}

static gboolean
cell_edited (GtkTreeModel		   *model,
             const gchar         *path_string,
             const gchar         *new_text,
				 gint 					column)
{
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;
	gchar* old_text;
	gboolean toggle_state;

	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, &iter, SCHEMES_CHECKBOX_COLUMN, &toggle_state, 
												 column, &old_text, -1);
	g_free (old_text);

	gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
							  g_strdup (new_text), -1);

	gtk_tree_path_free (path);

	return toggle_state;
}

static void
scheme_edited_cb (GtkCellRendererText *cell,
             const gchar         *path_string,
             const gchar         *new_text,
             gpointer             data)
{
	GtkTreeModel* model = GTK_TREE_MODEL (data);
	if (cell_edited (model, path_string, new_text, SCHEMES_KEYWORD_COLUMN))
	{
		//--> if column was checked, set the action has edited
		field_changed_cb (G_OBJECT (cell), NULL);
	}
}

static void
scheme_desc_edited_cb (GtkCellRendererText *cell,
             const gchar         *path_string,
             const gchar         *new_text,
             gpointer             data)
{
	GtkTreeModel* model = GTK_TREE_MODEL (data);
	cell_edited (model, path_string, new_text, SCHEMES_DESC_COLUMN);
}

static void
scheme_list_selection_changed_cb (GtkTreeSelection *selection, gpointer user_data)
{
	GtkWidget *delete_button = nact_get_glade_widget_from ("RemoveSchemeButton", GLADE_EDIT_DIALOG_WIDGET);

	if (gtk_tree_selection_count_selected_rows (selection) > 0) {
		gtk_widget_set_sensitive (delete_button, TRUE);
	} else {
		gtk_widget_set_sensitive (delete_button, FALSE);
	}
}

void add_scheme_clicked (GtkWidget* widget, gpointer user_data)
{
	GtkWidget* listview = nact_get_glade_widget_from ("SchemesTreeView", GLADE_EDIT_DIALOG_WIDGET);
	GtkTreeModel* model = gtk_tree_view_get_model (GTK_TREE_VIEW (listview));
	GtkTreeIter row;
	
	gtk_list_store_append (model, &row);
	gtk_list_store_set (model, &row, SCHEMES_CHECKBOX_COLUMN, FALSE, 
												SCHEMES_KEYWORD_COLUMN, _("new-scheme"),
												SCHEMES_DESC_COLUMN, _("New Scheme Description"), -1);
}

void remove_scheme_clicked (GtkWidget* widget, gpointer user_data)
{
	GtkWidget* listview = nact_get_glade_widget_from ("SchemesTreeView", GLADE_EDIT_DIALOG_WIDGET);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (listview));
	GtkTreeModel* model = gtk_tree_view_get_model (GTK_TREE_VIEW (listview));
	GList* selected_values_path = NULL;
	GtkTreeIter iter;
	GtkTreePath* path;
	GList* list_iter;
	gboolean toggle_state;

	selected_values_path = gtk_tree_selection_get_selected_rows (selection, &model);

	for (list_iter = selected_values_path; list_iter; list_iter = list_iter->next)
	{
		path = (GtkTreePath*)(list_iter->data);
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, SCHEMES_CHECKBOX_COLUMN, &toggle_state, -1);
		
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

		if (toggle_state)
		{
			//--> if column was checked, set the action has edited
			field_changed_cb (G_OBJECT (widget), NULL);
		}
	}
	
	g_list_foreach (selected_values_path, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (selected_values_path);
}

static void create_schemes_selection_list (void)
{
	GtkWidget* listview = nact_get_glade_widget_from ("SchemesTreeView", GLADE_EDIT_DIALOG_WIDGET);
	GSList* iter;
	GSList* schemes_list = nact_prefs_get_schemes_list ();
	GtkListStore* model;
	GtkTreeIter row;
	gchar* label;
	GtkTreeViewColumn *column;
	GtkCellRenderer* toggled_cell;
	GtkCellRenderer* text_cell;

	model = gtk_list_store_new (SCHEMES_N_COLUMN, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
	
	for (iter = schemes_list; iter; iter = iter->next)
	{
		gchar** tokens = g_strsplit ((gchar*)iter->data, "|", 2);
		
		gtk_list_store_append (model, &row);
		gtk_list_store_set (model, &row, SCHEMES_CHECKBOX_COLUMN, FALSE, 
													SCHEMES_KEYWORD_COLUMN, tokens[0],
												   SCHEMES_DESC_COLUMN, tokens[1], -1);
		
		g_strfreev (tokens);
	}

	g_slist_foreach (schemes_list, (GFunc)g_free, NULL);
	g_slist_free (schemes_list);

	gtk_tree_view_set_model (GTK_TREE_VIEW (listview), GTK_TREE_MODEL (model));
	g_object_unref (model);
	
	toggled_cell = gtk_cell_renderer_toggle_new ();

	g_signal_connect (G_OBJECT (toggled_cell), "toggled",
			  G_CALLBACK (scheme_selection_toggled_cb), 
			  gtk_tree_view_get_model (GTK_TREE_VIEW (listview)));

	column = gtk_tree_view_column_new_with_attributes ("",
							   toggled_cell,
							   "active", SCHEMES_CHECKBOX_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (listview), column);
	
	text_cell = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (text_cell), "editable", TRUE, NULL);

	g_signal_connect (G_OBJECT (text_cell), "edited",
							G_CALLBACK (scheme_edited_cb), 
							gtk_tree_view_get_model (GTK_TREE_VIEW (listview)));
	
	column = gtk_tree_view_column_new_with_attributes (_("Scheme"),
							   text_cell,
							   "text", SCHEMES_KEYWORD_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (listview), column);

	text_cell = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (text_cell), "editable", TRUE, NULL);

	g_signal_connect (G_OBJECT (text_cell), "edited",
							G_CALLBACK (scheme_desc_edited_cb), 
							gtk_tree_view_get_model (GTK_TREE_VIEW (listview)));
	
	column = gtk_tree_view_column_new_with_attributes (_("Description"),
							   text_cell,
							   "text", SCHEMES_DESC_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (listview), column);

	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (listview))), "changed",
			  G_CALLBACK (scheme_list_selection_changed_cb), NULL);
}

static gboolean reset_schemes_list (GtkTreeModel* scheme_model, GtkTreePath *path, 
											GtkTreeIter* iter, gpointer data)
{
	gtk_list_store_set (GTK_LIST_STORE (scheme_model), iter, SCHEMES_CHECKBOX_COLUMN, FALSE, -1);

	return FALSE; // Don't stop looping
}

static gboolean get_action_schemes_list (GtkTreeModel* scheme_model, GtkTreePath *path, 
													  GtkTreeIter* iter, gpointer data)
{
	GSList** list = data;
	gboolean toggle_state;
	gchar* scheme;

	gtk_tree_model_get (scheme_model, iter, SCHEMES_CHECKBOX_COLUMN, &toggle_state, 
														 SCHEMES_KEYWORD_COLUMN, &scheme, -1);

	if (toggle_state)
	{
		(*list) = g_slist_append ((*list), scheme);
	}
	else
	{
		g_free (scheme);
	}

	return FALSE; // Don't stop looping
}

static gboolean get_all_schemes_list (GtkTreeModel* scheme_model, GtkTreePath *path, 
													  GtkTreeIter* iter, gpointer data)
{
	GSList** list = data;
	gchar* scheme;
	gchar* desc;
	gchar* scheme_desc;

	gtk_tree_model_get (scheme_model, iter, SCHEMES_KEYWORD_COLUMN, &scheme, 
														 SCHEMES_DESC_COLUMN, &desc, -1);

	scheme_desc = g_strjoin ("|", scheme, desc, NULL);

	(*list) = g_slist_append ((*list), scheme_desc);

	g_free (scheme);
	g_free (desc);

	return FALSE; // Don't stop looping
}

static void set_action_schemes (gchar* action_scheme, GtkTreeModel* scheme_model)
{
	GtkTreeIter iter;
	gboolean iter_ok = FALSE;
	gboolean found = FALSE;
	gchar* scheme;

	iter_ok = gtk_tree_model_get_iter_first (scheme_model, &iter);

	while (iter_ok && !found)
	{
		gtk_tree_model_get (scheme_model, &iter, SCHEMES_KEYWORD_COLUMN, &scheme, -1);

		if (g_ascii_strcasecmp (action_scheme, scheme) == 0)
		{
			gtk_list_store_set (GTK_LIST_STORE (scheme_model), &iter, SCHEMES_CHECKBOX_COLUMN, TRUE, -1);
			found = TRUE;
		}

		g_free (scheme);

		iter_ok = gtk_tree_model_iter_next (scheme_model, &iter);
	}

	if (!found)
	{
		gtk_list_store_append (GTK_LIST_STORE (scheme_model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (scheme_model), &iter, SCHEMES_CHECKBOX_COLUMN, TRUE, 
													SCHEMES_KEYWORD_COLUMN, action_scheme,
												   SCHEMES_DESC_COLUMN, "", -1);
	}
}

static gboolean
open_editor (NautilusActionsConfigAction *action, gboolean is_new)
{
	static gboolean init = FALSE;
	GtkWidget* editor;
	GladeXML *gui;
	gboolean ret = FALSE;
	gchar *label;
	NautilusActionsConfigGconf *config;
	GList* aligned_widgets = NULL;
	GList* iter;
	GSList* list;
	GtkSizeGroup* label_size_group;
	GtkSizeGroup* button_size_group;
	GtkWidget *menu_icon, *scheme_listview;
	GtkWidget *menu_label, *menu_tooltip;
	GtkWidget *command_path, *command_params;
	GtkWidget *only_files, *only_folders, *both, *accept_multiple;
	gint width, height, x, y;
	GtkTreeModel* scheme_model;

	if (!init)
	{
		/* load the GUI */
		gui = nact_get_glade_xml_object (GLADE_EDIT_DIALOG_WIDGET);
		if (!gui) {
			nautilus_actions_display_error (_("Could not load interface for Nautilus Actions Config Tool"));
			return FALSE;
		}

		glade_xml_signal_autoconnect(gui);

		menu_icon = nact_get_glade_widget_from ("MenuIconComboBoxEntry", GLADE_EDIT_DIALOG_WIDGET);

		g_signal_connect (G_OBJECT (GTK_BIN (menu_icon)->child), "changed",
							   G_CALLBACK (preview_icon_changed_cb), NULL);
		
		fill_menu_icon_combo_list_of (GTK_COMBO_BOX_ENTRY (menu_icon));

		gtk_tooltips_set_tip (gtk_tooltips_new (), GTK_WIDGET (GTK_BIN (menu_icon)->child),
									 _("Icon of the menu item in the Nautilus contextual menu"), "");
	
		create_schemes_selection_list ();
		
		aligned_widgets = nact_get_glade_widget_prefix_from ("LabelAlign", GLADE_EDIT_DIALOG_WIDGET);
		label_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		for (iter = aligned_widgets; iter; iter = iter->next)
		{
			gtk_size_group_add_widget (label_size_group, GTK_WIDGET (iter->data));
		}
		button_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		gtk_size_group_add_widget (button_size_group, 
											nact_get_glade_widget_from ("IconBrowseButton", 
																		GLADE_EDIT_DIALOG_WIDGET));
		gtk_size_group_add_widget (button_size_group, 
											nact_get_glade_widget_from ("BrowseButton", 
																		GLADE_EDIT_DIALOG_WIDGET));
		gtk_size_group_add_widget (button_size_group, 
											nact_get_glade_widget_from ("LegendButton", 
																		GLADE_EDIT_DIALOG_WIDGET));
		/* free memory */
		g_object_unref (gui);
		init = TRUE;
	}

	editor = nact_get_glade_widget_from ("EditActionDialog", GLADE_EDIT_DIALOG_WIDGET);

	if (is_new)
	{
		gtk_window_set_title (GTK_WINDOW (editor), _("Add a New Action"));
	}
	else
	{
		gchar* title = g_strdup_printf (_("Edit Action \"%s\""), action->label);
		gtk_window_set_title (GTK_WINDOW (editor), title);
		g_free (title);
	}

	/* Get the default dialog size */
	gtk_window_get_default_size (GTK_WINDOW (editor), &width, &height);
	/* Override with preferred one, if any */
	nact_prefs_get_edit_dialog_size (&width, &height);
	
	gtk_window_resize (GTK_WINDOW (editor), width, height);

	if (nact_prefs_get_edit_dialog_position (&x, &y))
	{
		gtk_window_move (GTK_WINDOW (editor), x, y);
	}
	
	menu_label = nact_get_glade_widget_from ("MenuLabelEntry", GLADE_EDIT_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (menu_label), action->label);

	menu_tooltip = nact_get_glade_widget_from ("MenuTooltipEntry", GLADE_EDIT_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (menu_tooltip), action->tooltip);

	menu_icon = nact_get_glade_widget_from ("MenuIconComboBoxEntry", GLADE_EDIT_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (GTK_BIN (menu_icon)->child), action->icon);
	
	command_path = nact_get_glade_widget_from ("CommandPathEntry", GLADE_EDIT_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (command_path), action->path);

	command_params = nact_get_glade_widget_from ("CommandParamsEntry", GLADE_EDIT_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (command_params), action->parameters);

	only_folders = nact_get_glade_widget_from ("OnlyFoldersButton", GLADE_EDIT_DIALOG_WIDGET);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (only_folders), action->is_dir);

	only_files = nact_get_glade_widget_from ("OnlyFilesButton", GLADE_EDIT_DIALOG_WIDGET);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (only_files), action->is_file);

	both = nact_get_glade_widget_from ("BothButton", GLADE_EDIT_DIALOG_WIDGET);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (both), action->is_file && action->is_dir);

	accept_multiple = nact_get_glade_widget_from ("AcceptMultipleButton", GLADE_EDIT_DIALOG_WIDGET);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (accept_multiple), action->accept_multiple_files);

	scheme_listview = nact_get_glade_widget_from ("SchemesTreeView", GLADE_EDIT_DIALOG_WIDGET);
	scheme_model = gtk_tree_view_get_model (GTK_TREE_VIEW (scheme_listview));
	gtk_tree_model_foreach (scheme_model, (GtkTreeModelForeachFunc)reset_schemes_list, NULL);
	if (!is_new)
	{
		g_slist_foreach (action->schemes, (GFunc)set_action_schemes, scheme_model);
	}

	/* run the dialog */
	gtk_dialog_set_response_sensitive (GTK_DIALOG (editor), GTK_RESPONSE_OK, FALSE);
	switch (gtk_dialog_run (GTK_DIALOG (editor))) {
	case GTK_RESPONSE_OK :
		config = nautilus_actions_config_gconf_get ();
		label = (gchar*)gtk_entry_get_text (GTK_ENTRY (menu_label));
		if (is_new && nautilus_actions_config_get_action (NAUTILUS_ACTIONS_CONFIG (config), label)) {
			gchar *str;

			str = g_strdup_printf (_("There is already an action named %s. Please choose another name"), label);
			nautilus_actions_display_error (str);
			g_free (str);
			ret = FALSE;
		} else {
			nautilus_actions_config_action_set_label (action, label);
			nautilus_actions_config_action_set_tooltip (action, gtk_entry_get_text (GTK_ENTRY (menu_tooltip)));
			nautilus_actions_config_action_set_icon (action, gtk_entry_get_text (GTK_ENTRY (GTK_BIN (menu_icon)->child)));
			nautilus_actions_config_action_set_path (action, gtk_entry_get_text (GTK_ENTRY (command_path)));
			nautilus_actions_config_action_set_parameters (action, gtk_entry_get_text (GTK_ENTRY (command_params)));

			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (only_files))) {
				nautilus_actions_config_action_set_is_file (action, TRUE);
				nautilus_actions_config_action_set_is_dir (action, FALSE);
			} else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (only_folders))) {
				nautilus_actions_config_action_set_is_file (action, FALSE);
				nautilus_actions_config_action_set_is_dir (action, TRUE);
			} else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (both))) {
				nautilus_actions_config_action_set_is_file (action, TRUE);
				nautilus_actions_config_action_set_is_dir (action, TRUE);
			}

			nautilus_actions_config_action_set_accept_multiple (
				action, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (accept_multiple)));

			list = NULL;
			gtk_tree_model_foreach (scheme_model, (GtkTreeModelForeachFunc)get_action_schemes_list, &list);
			nautilus_actions_config_action_set_schemes (action, list);
			g_slist_foreach (list, (GFunc) g_free, NULL);
			g_slist_free (list);

			if (is_new)
				ret = nautilus_actions_config_add_action (NAUTILUS_ACTIONS_CONFIG (config), action);
			else
				ret = nautilus_actions_config_update_action (NAUTILUS_ACTIONS_CONFIG (config), action);
		}
		break;
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL :
		ret = FALSE;
		break;
	}
	
	// Save preferences
	list = NULL;
	gtk_tree_model_foreach (scheme_model, (GtkTreeModelForeachFunc)get_all_schemes_list, &list);
	nact_prefs_set_schemes_list (list);
	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);

	nact_prefs_set_edit_dialog_size (GTK_WINDOW (editor));
	nact_prefs_set_edit_dialog_position (GTK_WINDOW (editor));

	gtk_widget_hide (editor);

	return ret;
}

gboolean
nact_editor_new_action (void)
{
	gboolean val;
	NautilusActionsConfigAction *action = nautilus_actions_config_action_new_default ();

	val = open_editor (action, TRUE);
	nautilus_actions_config_action_free (action);

	return val;
}

gboolean
nact_editor_edit_action (NautilusActionsConfigAction *action)
{
	return open_editor (action, FALSE);
}

// vim:ts=3:sw=3:tw=1024:cin
