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

#ifndef __UI_FMA_MAIN_WINDOW_H__
#define __UI_FMA_MAIN_WINDOW_H__

/**
 * SECTION: main-window
 * @title: FMAMainWindow
 * @short_description: The Main Window class definition
 * @include: ui/fma-main-window.h
 */

#include "fma-application.h"
#include "fma-clipboard.h"
#include "fma-main-window-def.h"
#include "fma-sort-buttons.h"
#include "fma-status-bar.h"
#include "fma-tree-view.h"

G_BEGIN_DECLS

/**
 * Signals defined on the main window
 */
#define MAIN_SIGNAL_ITEM_UPDATED            "main-item-updated"
#define MAIN_SIGNAL_UPDATE_SENSITIVITIES	"main-signal-update-sensitivities"

/**
 * The data which, when modified, should be redisplayed asap.
 * This is used by MAIN_SIGNAL_ITEM_UPDATED signal.
 */
enum {
	MAIN_DATA_LABEL    = 1<<0,
	MAIN_DATA_ICON     = 1<<1,
	MAIN_DATA_PROVIDER = 1<<2,
};

/**
 * Properties set against the main window
 */
#define MAIN_PROP_ITEM						"main-current-item"
#define MAIN_PROP_PROFILE					"main-current-profile"
#define MAIN_PROP_CONTEXT					"main-current-context"
#define MAIN_PROP_EDITABLE					"main-editable"
#define MAIN_PROP_REASON					"main-reason"

FMAMainWindow  *fma_main_window_new             ( FMAApplication *application );

FMAClipboard   *fma_main_window_get_clipboard   ( const FMAMainWindow *window );

FMASortButtons *fma_main_window_get_sort_buttons( const FMAMainWindow *window );

FMAStatusBar   *fma_main_window_get_statusbar   ( const FMAMainWindow *window );

FMATreeView    *fma_main_window_get_items_view  ( const FMAMainWindow *window );

void            fma_main_window_reload          ( FMAMainWindow *window );

void            fma_main_window_block_reload    ( FMAMainWindow *window );

gboolean        fma_main_window_dispose_has_run ( const FMAMainWindow *window );

gboolean        fma_main_window_quit            ( FMAMainWindow *window );

G_END_DECLS

#endif /* __UI_FMA_MAIN_WINDOW_H__ */
