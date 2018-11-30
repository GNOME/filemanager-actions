/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __UI_FMA_TREE_VIEW_H__
#define __UI_FMA_TREE_VIEW_H__

/*
 * SECTION: fma-tree-view
 * @title: FMATreeView
 * @short_description: The Tree View Base Class Definition
 * @include: ui/fma-tree-view.h
 *
 * This is a convenience class to manage a read-only items tree view.
 *
 * The FMATreeView encapsulates the GtkTreeView which displays the items
 * list on the left of the main pane.
 *
 * It is instantiated from FMAMainWindow::on_initialize_gtk().
 *
 * A pointer to this FMATreeView is attached to the FMAMainWindow at
 * construction time.
 */

#include "api/fma-object-item.h"

#include "base-window.h"
#include "fma-main-window-def.h"

G_BEGIN_DECLS

#define FMA_TYPE_TREE_VIEW                ( fma_tree_view_get_type())
#define FMA_TREE_VIEW( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_TREE_VIEW, FMATreeView ))
#define FMA_TREE_VIEW_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_TREE_VIEW, FMATreeViewClass ))
#define FMA_IS_TREE_VIEW( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_TREE_VIEW ))
#define FMA_IS_TREE_VIEW_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_TREE_VIEW ))
#define FMA_TREE_VIEW_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_TREE_VIEW, FMATreeViewClass ))

typedef struct _FMATreeViewPrivate        FMATreeViewPrivate;

typedef struct {
	/*< private >*/
	GtkBin              parent;
	FMATreeViewPrivate *private;
}
	FMATreeView;

typedef struct {
	/*< private >*/
	GtkBinClass         parent;
}
	FMATreeViewClass;

/**
 * Signals emitted by the FMATreeView instance.
 */
#define TREE_SIGNAL_COUNT_CHANGED				"tree-signal-count-changed"
#define TREE_SIGNAL_FOCUS_IN					"tree-signal-focus-in"
#define TREE_SIGNAL_FOCUS_OUT					"tree-signal-focus-out"
#define TREE_SIGNAL_LEVEL_ZERO_CHANGED			"tree-signal-level-zero-changed"
#define TREE_SIGNAL_MODIFIED_STATUS_CHANGED		"tree-signal-modified-status-changed"
#define TREE_SIGNAL_SELECTION_CHANGED		    "tree-selection-changed"
#define TREE_SIGNAL_CONTEXT_MENU			    "tree-signal-open-popup"

typedef enum {
	TREE_MODE_EDITION = 1,				/* main window */
	TREE_MODE_SELECTION,				/* export assistant */
	/*< private >*/
	TREE_MODE_N_MODES
}
	NactTreeMode;

/**
 * When getting a list of items; these indicators may be OR-ed.
 */
enum {
	TREE_LIST_SELECTED = 1<<0,
	TREE_LIST_MODIFIED = 1<<1,
	TREE_LIST_ALL      = 1<<7,
	TREE_LIST_DELETED  = 1<<8,
};

GType          fma_tree_view_get_type          ( void );

FMATreeView   *fma_tree_view_new               ( FMAMainWindow *main_window );

void           fma_tree_view_set_mnemonic      ( FMATreeView *view,
														GtkContainer *parent,
														const gchar *widget_name );

void           fma_tree_view_set_edition_mode  ( FMATreeView *view,
														NactTreeMode mode );

void           fma_tree_view_fill              ( FMATreeView *view, GList *items );

gboolean       fma_tree_view_are_notify_allowed( const FMATreeView *view );
void           fma_tree_view_set_notify_allowed( FMATreeView *view, gboolean allow );

void           fma_tree_view_collapse_all      ( const FMATreeView *view );
void           fma_tree_view_expand_all        ( const FMATreeView *view );
FMAObjectItem *fma_tree_view_get_item_by_id    ( const FMATreeView *view, const gchar *id );
GList         *fma_tree_view_get_items         ( const FMATreeView *view );
GList         *fma_tree_view_get_items_ex      ( const FMATreeView *view, guint mode );

void           fma_tree_view_select_row_at_path( FMATreeView *view, GtkTreePath *path );

G_END_DECLS

#endif /* __UI_FMA_TREE_VIEW_H__ */
