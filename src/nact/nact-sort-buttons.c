/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

#include <core/na-iprefs.h>
#include <core/na-updater.h>

#include "nact-application.h"
#include "nact-sort-buttons.h"

static void on_sort_down_button_toggled( GtkToggleButton *button, NactMainWindow *window );
static void on_sort_manual_button_toggled( GtkToggleButton *button, NactMainWindow *window );
static void on_sort_up_button_toggled( GtkToggleButton *button, NactMainWindow *window );
static void set_new_sort_order( NactMainWindow *window, guint order_mode );
static void display_sort_order( NactMainWindow *window, guint order_mode );

/**
 * nact_sort_buttons_initial_load:
 * @window: the #NactMainWindow.
 *
 * Initial loading of the UI. This is done only once.
 */
void
nact_sort_buttons_initial_load( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_sort_buttons_initial_load";

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
}

/**
 * nact_sort_buttons_runtime_init:
 * @window: the #NactMainWindow.
 *
 * Initialization of the UI each time it is displayed.
 */
void
nact_sort_buttons_runtime_init( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_sort_buttons_runtime_init";
	GtkToggleButton *button;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortUpButton" ));
	gtk_toggle_button_set_active( button, FALSE );
	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( button ),
			"toggled",
			G_CALLBACK( on_sort_up_button_toggled ));

	button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortDownButton" ));
	gtk_toggle_button_set_active( button, FALSE );
	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( button ),
			"toggled",
			G_CALLBACK( on_sort_down_button_toggled ));

	button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortManualButton" ));
	gtk_toggle_button_set_active( button, FALSE );
	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( button ),
			"toggled",
			G_CALLBACK( on_sort_manual_button_toggled ));
}

/**
 * nact_sort_buttons_all_widgets_showed:
 * @window: the #NactMainWindow.
 *
 * Called when all the widgets are showed after end of all runtime
 * initializations.
 */
void
nact_sort_buttons_all_widgets_showed( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_sort_buttons_all_widgets_showed";
	NactApplication *application;
	NAUpdater *updater;
	guint order_mode;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	updater = nact_application_get_updater( application );
	order_mode = na_iprefs_get_order_mode( NA_IPREFS( updater ));
	display_sort_order( window, order_mode );
}

/**
 * nact_sort_buttons_dispose:
 * @window: the #NactMainWindow.
 *
 * The main window is disposed.
 */
void
nact_sort_buttons_dispose( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_sort_buttons_dispose";

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
}

/**
 * nact_sort_buttons_display_order_change:
 * @window: the #NactMainWindow.
 * @order_mode: the new order mode.
 *
 * Relayed via NactMainWindow, this is a NAIPivotConsumer notification.
 */
void
nact_sort_buttons_display_order_change( NactMainWindow *window, guint order_mode )
{
	static const gchar *thisfn = "nact_sort_buttons_display_order_change";
	GtkToggleButton *button;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortUpButton" ));
	gtk_toggle_button_set_active( button, FALSE );

	button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortDownButton" ));
	gtk_toggle_button_set_active( button, FALSE );

	button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortManualButton" ));
	gtk_toggle_button_set_active( button, FALSE );

	display_sort_order( window, order_mode );
}

static void
on_sort_down_button_toggled( GtkToggleButton *toggled_button, NactMainWindow *window )
{
	GtkToggleButton *button;

	if( gtk_toggle_button_get_active( toggled_button )){

		button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortUpButton" ));
		gtk_toggle_button_set_active( button, FALSE );

		button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortManualButton" ));
		gtk_toggle_button_set_active( button, FALSE );

		set_new_sort_order( window, IPREFS_ORDER_ALPHA_DESCENDING );
	}
}

static void
on_sort_manual_button_toggled( GtkToggleButton *toggled_button, NactMainWindow *window )
{
	GtkToggleButton *button;

	if( gtk_toggle_button_get_active( toggled_button )){

		button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortUpButton" ));
		gtk_toggle_button_set_active( button, FALSE );

		button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortDownButton" ));
		gtk_toggle_button_set_active( button, FALSE );

		set_new_sort_order( window, IPREFS_ORDER_MANUAL );
	}
}

static void
on_sort_up_button_toggled( GtkToggleButton *toggled_button, NactMainWindow *window )
{
	GtkToggleButton *button;

	if( gtk_toggle_button_get_active( toggled_button )){

		button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortDownButton" ));
		gtk_toggle_button_set_active( button, FALSE );

		button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortManualButton" ));
		gtk_toggle_button_set_active( button, FALSE );

		set_new_sort_order( window, IPREFS_ORDER_ALPHA_ASCENDING );
	}
}

static void
set_new_sort_order( NactMainWindow *window, guint order_mode )
{
	NactApplication *application;
	NAUpdater *updater;

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	updater = nact_application_get_updater( application );
	na_iprefs_set_order_mode( NA_IPREFS( updater ), order_mode );
}

static void
display_sort_order( NactMainWindow *window, guint order_mode )
{
	GtkToggleButton *button;

	switch( order_mode ){
		case IPREFS_ORDER_ALPHA_ASCENDING:
			button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortUpButton" ));
			gtk_toggle_button_set_active( button, TRUE );
			break;

		case IPREFS_ORDER_ALPHA_DESCENDING:
			button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortDownButton" ));
			gtk_toggle_button_set_active( button, TRUE );
			break;

		case IPREFS_ORDER_MANUAL:
			button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "SortManualButton" ));
			gtk_toggle_button_set_active( button, TRUE );
			break;

	}
}
