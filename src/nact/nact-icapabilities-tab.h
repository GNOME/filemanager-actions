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

#ifndef __NACT_ICAPABILITIES_TAB_H__
#define __NACT_ICAPABILITIES_TAB_H__

/**
 * SECTION: nact_icapabilities_tab
 * @short_description: #NactICapabilitiesTab interface declaration.
 * @include: nact/nact-icapabilities-tab.h
 *
 * This interface implements all the widgets which define the
 * conditions for the action.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NACT_ICAPABILITIES_TAB_TYPE							( nact_icapabilities_tab_get_type())
#define NACT_ICAPABILITIES_TAB( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_ICAPABILITIES_TAB_TYPE, NactICapabilitiesTab ))
#define NACT_IS_ICAPABILITIES_TAB( object )					( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_ICAPABILITIES_TAB_TYPE ))
#define NACT_ICAPABILITIES_TAB_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_ICAPABILITIES_TAB_TYPE, NactICapabilitiesTabInterface ))

typedef struct NactICapabilitiesTab                 NactICapabilitiesTab;

typedef struct NactICapabilitiesTabInterfacePrivate NactICapabilitiesTabInterfacePrivate;

typedef struct {
	GTypeInterface                        parent;
	NactICapabilitiesTabInterfacePrivate *private;
}
	NactICapabilitiesTabInterface;

GType   nact_icapabilities_tab_get_type( void );

void    nact_icapabilities_tab_initial_load_toplevel( NactICapabilitiesTab *instance );
void    nact_icapabilities_tab_runtime_init_toplevel( NactICapabilitiesTab *instance );
void    nact_icapabilities_tab_all_widgets_showed   ( NactICapabilitiesTab *instance );
void    nact_icapabilities_tab_dispose              ( NactICapabilitiesTab *instance );

G_END_DECLS

#endif /* __NACT_ICAPABILITIES_TAB_H__ */