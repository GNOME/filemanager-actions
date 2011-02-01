/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "nact-iactions-list.h"
#include "nact-main-toolbar.h"
#include "nact-main-menubar-view.h"
#include "nact-menubar-priv.h"

static void on_view_toolbar_activated( GtkToggleAction *action, NactMainWindow *window, int toolbar_id );

/**
 * nact_main_menubar_view_on_update_sensitivities:
 * @window: the #NactMainWindow main window.
 * user_data: the data passed to the function via the signal.
 * @mis: the #MenubarIndicatorsStruct struct.
 *
 * Update sensitivity of items of the View menu.
 */
void
nact_main_menubar_view_on_update_sensitivities( NactMenubar *bar )
{
	guint count_list;

	/* expand all/collapse all requires at least one item in the list */
	count_list = bar->private->list_menus + bar->private->list_actions + bar->private->list_profiles;
	nact_menubar_enable_item( bar, "ExpandAllItem", count_list > 0 );
	nact_menubar_enable_item( bar, "CollapseAllItem", count_list > 0 );
}

/**
 * nact_main_menubar_view_on_expand_all:
 * @gtk_action: the #GtkAction action.
 * @window: the #NactMainWindow main window.
 *
 * Triggers View / Expand all item.
 */
void
nact_main_menubar_view_on_expand_all( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_iactions_list_expand_all( NACT_IACTIONS_LIST( window ));
}

/**
 * nact_main_menubar_view_on_collapse_all:
 * @gtk_action: the #GtkAction action.
 * @window: the #NactMainWindow main window.
 *
 * Triggers View / Collapse all item.
 */
void
nact_main_menubar_view_on_collapse_all( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_iactions_list_collapse_all( NACT_IACTIONS_LIST( window ));
}

/**
 * nact_main_menubar_view_on_toolbar_file:
 * @gtk_action: the #GtkAction action.
 * @window: the #NactMainWindow main window.
 *
 * Triggers View / Toolbar / File item.
 */
void
nact_main_menubar_view_on_toolbar_file( GtkToggleAction *action, NactMainWindow *window )
{
	/*on_view_toolbar_activated( action, window, MENUBAR_IPREFS_FILE_TOOLBAR, "/ui/FileToolbar", MENUBAR_FILE_TOOLBAR_POS );*/
	on_view_toolbar_activated( action, window, MAIN_TOOLBAR_FILE_ID );
}

/**
 * nact_main_menubar_view_on_toolbar_edit:
 * @gtk_action: the #GtkAction action.
 * @window: the #NactMainWindow main window.
 *
 * Triggers View / Toolbar / Edit item.
 */
void
nact_main_menubar_view_on_toolbar_edit( GtkToggleAction *action, NactMainWindow *window )
{
	/*on_view_toolbar_activated( action, window, MENUBAR_IPREFS_EDIT_TOOLBAR, "/ui/EditToolbar", MENUBAR_EDIT_TOOLBAR_POS );*/
	on_view_toolbar_activated( action, window, MAIN_TOOLBAR_EDIT_ID );
}

/**
 * nact_main_menubar_view_on_toolbar_tools:
 * @gtk_action: the #GtkAction action.
 * @window: the #NactMainWindow main window.
 *
 * Triggers View / Toolbar / Tools item.
 */
void
nact_main_menubar_view_on_toolbar_tools( GtkToggleAction *action, NactMainWindow *window )
{
	/*on_view_toolbar_activated( action, window, MENUBAR_IPREFS_TOOLS_TOOLBAR, "/ui/ToolsToolbar", MENUBAR_TOOLS_TOOLBAR_POS );*/
	on_view_toolbar_activated( action, window, MAIN_TOOLBAR_TOOLS_ID );
}

/**
 * nact_main_menubar_view_on_toolbar_help:
 * @gtk_action: the #GtkAction action.
 * @window: the #NactMainWindow main window.
 *
 * Triggers View / Toolbar / Help item.
 */
void
nact_main_menubar_view_on_toolbar_help( GtkToggleAction *action, NactMainWindow *window )
{
	/*on_view_toolbar_activated( action, window, MENUBAR_IPREFS_HELP_TOOLBAR, "/ui/HelpToolbar", MENUBAR_HELP_TOOLBAR_POS );*/
	on_view_toolbar_activated( action, window, MAIN_TOOLBAR_HELP_ID );
}

static void
on_view_toolbar_activated( GtkToggleAction *action, NactMainWindow *window, int toolbar_id )
{
	gboolean is_active;

	BAR_WINDOW_VOID( window );

	is_active = gtk_toggle_action_get_active( action );

	nact_main_toolbar_activate( window, toolbar_id, bar->private->ui_manager, is_active );
}
