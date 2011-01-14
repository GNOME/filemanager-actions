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

#include <core/na-iprefs.h>

#include "nact-application.h"
#include "nact-iprefs.h"

/* private interface data
 */
struct NactIPrefsInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;

static GType       register_type( void );
static void        interface_base_init( NactIPrefsInterface *klass );
static void        interface_base_finalize( NactIPrefsInterface *klass );

#if 0
static GConfValue *get_value( GConfClient *client, const gchar *path, const gchar *entry );
static void        set_value( GConfClient *client, const gchar *path, const gchar *entry, GConfValue *value );
#endif

GType
nact_iprefs_get_type( void )
{
	static GType iface_type = 0;

	if( !iface_type ){
		iface_type = register_type();
	}

	return( iface_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_iprefs_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIPrefsInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIPrefs", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NactIPrefsInterface *klass )
{
	static const gchar *thisfn = "nact_iprefs_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIPrefsInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIPrefsInterface *klass )
{
	static const gchar *thisfn = "nact_iprefs_interface_base_finalize";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_finalized = TRUE;

		g_free( klass->private );
	}
}

/**
 * nact_iprefs_get_export_format:
 * @window: this #BaseWindow-derived window.
 * @name: name of the export format key to be readen
 *
 * Returns: the export format currently set as a #GQuark.
 *
 * Used to default to export as a GConfEntry.
 * Starting with 3.1.0, defaults to Desktop1 (see. #nact-iprefs.h)
 *
 * Note: please take care of keeping the default value synchronized with
 * those defined in na-settings.c for 'export-preferred-format' key.
 */
GQuark
nact_iprefs_get_export_format( const BaseWindow *window, const gchar *name )
{
	GQuark export_format;
	NactApplication *application;
	NAUpdater *updater;
	NASettings *settings;
	gchar *format_str;

	export_format = g_quark_from_static_string( NA_IPREFS_DEFAULT_EXPORT_FORMAT );

	g_return_val_if_fail( BASE_IS_WINDOW( window ), export_format );

	if( st_initialized && !st_finalized ){

		application = NACT_APPLICATION( base_window_get_application( window ));
		updater = nact_application_get_updater( application );
		settings = na_pivot_get_settings( NA_PIVOT( updater ));
		format_str = na_settings_get_string( settings, name, NULL, NULL );

		if( format_str ){
			export_format = g_quark_from_string( format_str );
			g_free( format_str );
		}
	}

	return( export_format );
}

/**
 * nact_iprefs_set_export_format:
 * @window: this #BaseWindow-derived window.
 * @format: the new value to be written.
 *
 * Writes the preferred export format' to the preference system.
 */
void
nact_iprefs_set_export_format( const BaseWindow *window, const gchar *name, GQuark format )
{
	NactApplication *application;
	NAUpdater *updater;
	NASettings *settings;

	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( st_initialized && !st_finalized ){

		application = NACT_APPLICATION( base_window_get_application( window ));
		updater = nact_application_get_updater( application );
		settings = na_pivot_get_settings( NA_PIVOT( updater ));
		na_settings_set_string( settings, name, g_quark_to_string( format ));
	}
}

/**
 * nact_iprefs_migrate_key:
 * @window: a #BaseWindow window.
 * @old_key: the old preference entry.
 * @new_key: the new preference entry.
 *
 * Migrates the content of an entry from an obsoleted key to a new one.
 * Removes the old key, along with the schema associated to it,
 * considering that the version which asks for this migration has
 * installed a schema corresponding to the new key.
 */
void
nact_iprefs_migrate_key( const BaseWindow *window, const gchar *old_key, const gchar *new_key )
{
#if 0
	static const gchar *thisfn = "nact_iprefs_migrate_key";
	GConfClient *gconf_client;
	GConfValue *value;

	g_debug( "%s: window=%p, old_key=%s, new_key=%s", thisfn, ( void * ) window, old_key, new_key );
	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( st_initialized && !st_finalized ){

		gconf_client = NACT_IPREFS_GET_INTERFACE( window )->private->client;

		value = get_value( gconf_client, IPREFS_GCONF_PREFS_PATH, new_key );
		if( !value ){
			value = get_value( gconf_client, IPREFS_GCONF_PREFS_PATH, old_key );
			if( value ){
				set_value( gconf_client, IPREFS_GCONF_PREFS_PATH, new_key, value );
				gconf_value_free( value );
			}
		}

		/* do not remove entries which may still be used by an older N-A version
		 */
	}
#endif
}

/**
 * nact_iprefs_write_bool:
 * @window: this #BaseWindow-derived window.
 * @name: the preference entry.
 * @value: the value to be written.
 *
 * Writes the given boolean value.
 */
void
nact_iprefs_write_bool( const BaseWindow *window, const gchar *name, gboolean value )
{
	NactApplication *application;
	NAUpdater *updater;
	NASettings *settings;

	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( NACT_IS_IPREFS( window ));

	if( st_initialized && !st_finalized ){

		application = NACT_APPLICATION( base_window_get_application( window ));
		updater = nact_application_get_updater( application );
		settings = na_pivot_get_settings( NA_PIVOT( updater ));
		na_settings_set_boolean( settings, name, value );
	}
}

/**
 * nact_iprefs_write_uint:
 * @window: this #BaseWindow-derived window.
 * @name: the preference entry.
 * @value: the value to be written.
 *
 * Writes the given uint value.
 */
void
nact_iprefs_write_uint( const BaseWindow *window, const gchar *name, guint value )
{
	NactApplication *application;
	NAUpdater *updater;
	NASettings *settings;

	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( NACT_IS_IPREFS( window ));

	if( st_initialized && !st_finalized ){

		application = NACT_APPLICATION( base_window_get_application( window ));
		updater = nact_application_get_updater( application );
		settings = na_pivot_get_settings( NA_PIVOT( updater ));
		na_settings_set_uint( settings, name, value );
	}
}

/**
 * nact_iprefs_write_string:
 * @window: this #BaseWindow-derived window.
 * @name: the preference key.
 * @value: the value to be written.
 *
 * Writes the value as the given GConf preference.
 */
void
nact_iprefs_write_string( const BaseWindow *window, const gchar *name, const gchar *value )
{
	NactApplication *application;
	NAUpdater *updater;
	NASettings *settings;

	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( NACT_IS_IPREFS( window ));

	if( st_initialized && !st_finalized ){

		application = NACT_APPLICATION( base_window_get_application( window ));
		updater = nact_application_get_updater( application );
		settings = na_pivot_get_settings( NA_PIVOT( updater ));
		na_settings_set_string( settings, name, value );
	}
}

#if 0
static GConfValue *
get_value( GConfClient *client, const gchar *path, const gchar *entry )
{
	static const gchar *thisfn = "na_iprefs_get_value";
	GError *error = NULL;
	gchar *fullpath;
	GConfValue *value;

	fullpath = gconf_concat_dir_and_key( path, entry );

	value = gconf_client_get_without_default( client, fullpath, &error );

	if( error ){
		g_warning( "%s: key=%s, %s", thisfn, fullpath, error->message );
		g_error_free( error );
		if( value ){
			gconf_value_free( value );
			value = NULL;
		}
	}

	g_free( fullpath );

	return( value );
}

static void
set_value( GConfClient *client, const gchar *path, const gchar *entry, GConfValue *value )
{
	static const gchar *thisfn = "na_iprefs_set_value";
	GError *error = NULL;
	gchar *fullpath;

	g_return_if_fail( value );

	fullpath = gconf_concat_dir_and_key( path, entry );

	gconf_client_set( client, fullpath, value, &error );

	if( error ){
		g_warning( "%s: key=%s, %s", thisfn, fullpath, error->message );
		g_error_free( error );
	}

	g_free( fullpath );
}
#endif
