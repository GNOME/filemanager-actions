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

#ifndef __PLUGIN_TRACKER_FMA_TRACKER_PLUGIN_H__
#define __PLUGIN_TRACKER_FMA_TRACKER_PLUGIN_H__

/**
 * SECTION: fma_tracker_plugin
 * @short_description: #FMATrackerPlugin class definition.
 * @include: tracker/fma-tracker-plugin.h
 *
 * The #FMATrackerPlugin object is instantiated when Nautilus file manager loads
 * this plugin (this is the normal behavior of Nautilus to instanciate one
 * object of each plugin type).
 *
 * There is so only one #FMATrackerPlugin object in the process. As any Nautilus
 * extension, it is instantiated when the module is loaded by the file
 * manager, usually at startup time.
 *
 * The #FMATrackerPlugin object instanciates and keeps a new GDBusObjectManagerServer
 * rooted on our D-Bus path.
 * It then allocates an object at this same path, and another object which
 * implements the .Properties1 interface. Last connects to the method signal
 * before connecting the server to the session D-Bus.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define FMA_TYPE_TRACKER_PLUGIN                ( fma_tracker_plugin_get_type())
#define FMA_TRACKER_PLUGIN( object )           ( G_TYPE_CHECK_INSTANCE_CAST(( object ), FMA_TYPE_TRACKER_PLUGIN, FMATrackerPlugin ))
#define FMA_TRACKER_PLUGIN_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST(( klass ), FMA_TYPE_TRACKER_PLUGIN, FMATrackerPluginClass ))
#define FMA_IS_TRACKER_PLUGIN( object )        ( G_TYPE_CHECK_INSTANCE_TYPE(( object ), FMA_TYPE_TRACKER_PLUGIN ))
#define FMA_IS_TRACKER_PLUGIN_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_TRACKER_PLUGIN ))
#define FMA_TRACKER_PLUGIN_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_TRACKER_PLUGIN, FMATrackerPluginClass ))

typedef struct _FMATrackerPluginPrivate        FMATrackerPluginPrivate;

typedef struct {
	/*< private >*/
	GObject                  parent;
	FMATrackerPluginPrivate *private;
}
	FMATrackerPlugin;

typedef struct _FMATrackerPluginClassPrivate   FMATrackerPluginClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass                  parent;
	FMATrackerPluginClassPrivate *private;
}
	FMATrackerPluginClass;

GType    fma_tracker_plugin_get_type          ( void );
void     fma_tracker_plugin_register_type     ( GTypeModule *module );

G_END_DECLS

#endif /* __PLUGIN_TRACKER_FMA_TRACKER_PLUGIN_H__ */
