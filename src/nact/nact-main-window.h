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

#ifndef __NACT_MAIN_WINDOW_H__
#define __NACT_MAIN_WINDOW_H__

/**
 * SECTION: main-window
 * @title: NactMainWindow
 * @short_description: The Main Window class definition
 * @include: nact-main-window.h
 *
 * This class is derived from NactWindow and manages the main window.
 *
 * It embeds:
 * - the menubar,
 * - the toolbar,
 * - a button bar with sort buttons,
 * - the hierarchical list of items,
 * - a notebook which displays the content of the current item,
 * - a status bar.
 */

#include "nact-application.h"
#include "nact-clipboard.h"
#include "nact-window.h"
#include "nact-tree-view.h"

G_BEGIN_DECLS

#define NACT_MAIN_WINDOW_TYPE                ( nact_main_window_get_type())
#define NACT_MAIN_WINDOW( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_MAIN_WINDOW_TYPE, NactMainWindow ))
#define NACT_MAIN_WINDOW_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_MAIN_WINDOW_TYPE, NactMainWindowClass ))
#define NACT_IS_MAIN_WINDOW( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_MAIN_WINDOW_TYPE ))
#define NACT_IS_MAIN_WINDOW_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_MAIN_WINDOW_TYPE ))
#define NACT_MAIN_WINDOW_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_MAIN_WINDOW_TYPE, NactMainWindowClass ))

typedef struct _NactMainWindowPrivate        NactMainWindowPrivate;

typedef struct {
	/*< private >*/
	NactWindow             parent;
	NactMainWindowPrivate *private;
}
	NactMainWindow;

typedef struct _NactMainWindowClassPrivate   NactMainWindowClassPrivate;

typedef struct {
	/*< private >*/
	NactWindowClass             parent;
	NactMainWindowClassPrivate *private;
}
	NactMainWindowClass;

#define MAIN_SIGNAL_SELECTION_CHANGED					"main-selection-changed"

#define MAIN_WINDOW_SIGNAL_LEVEL_ZERO_ORDER_CHANGED		"main-window-level-zero-order-changed"

GType           nact_main_window_get_type( void );

NactMainWindow *nact_main_window_new           ( const NactApplication *application );

NactClipboard  *nact_main_window_get_clipboard ( const NactMainWindow *window );
NactTreeView   *nact_main_window_get_items_view( const NactMainWindow *window );

void            nact_main_window_move_to_deleted   ( NactMainWindow *window, GList *items );
void            nact_main_window_reload            ( NactMainWindow *window );
gboolean        nact_main_window_remove_deleted    ( NactMainWindow *window, GSList **messages );

gboolean        nact_main_window_quit              ( NactMainWindow *window );

G_END_DECLS

#endif /* __NACT_MAIN_WINDOW_H__ */
