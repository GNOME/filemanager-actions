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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "fma-desktop-environment.h"

static const FMADesktopEnv st_desktops[] = {
	{ DESKTOP_GNOME, N_( "GNOME desktop" ) },
	{ DESKTOP_KDE,   N_( "KDE desktop" ) },
	{ DESKTOP_LXDE,  N_( "LXDE desktop" ) },
	{ DESKTOP_ROX,   N_( "ROX desktop" ) },
	{ DESKTOP_XFCE,  N_( "XFCE desktop" ) },
	{ DESKTOP_OLD,   N_( "Legacy systems" ) },
	{ NULL }
};

/*
 * fma_desktop_environment_get_known_list:
 *
 * Returns the list of known desktop environments as defined by the
 * corresponding XDG specification.
 */
const FMADesktopEnv *
fma_desktop_environment_get_known_list( void )
{
	return(( const FMADesktopEnv * ) st_desktops );
}

/*
 * fma_desktop_environment_detect_running_desktop:
 *
 * Have asked on xdg-list how to identify the currently running desktop environment
 * (see http://standards.freedesktop.org/menu-spec/latest/apb.html)
 * For now, just reproduce the xdg-open algorythm from xdg-utils 1.0
 */
const gchar *
fma_desktop_environment_detect_running_desktop( void )
{
	static const gchar *thisfn = "fma_desktop_environment_detect_running_desktop";
	const gchar *value;
	gchar *output_str, *error_str;
	gint exit_status;
	GError *error;
	gboolean ok;
	int i;

	value = g_getenv( "XDG_CURRENT_DESKTOP" );
	if( value && strlen( value )){
		for( i = 0 ; st_desktops[i].id ; ++i ){
			if( !strcmp( st_desktops[i].id, value )){
				return( st_desktops[i].id );
			}
		}
	}

	value = g_getenv( "KDE_FULL_SESSION" );
	if( value && !strcmp( value, "true" )){
		return( DESKTOP_KDE );
	}

	/* GNOME_DESKTOP_SESSION_ID=this-is-deprecated
	 */
	value = g_getenv( "GNOME_DESKTOP_SESSION_ID" );
	if( value && strlen( value )){
		return( DESKTOP_GNOME );
	}

	value = g_getenv( "DESKTOP_SESSION" );
	if( value ){
		if( !strcmp( value, "gnome" )){
			return( DESKTOP_GNOME );
		}
		if( !strcmp( value, "xfce" )){
			return( DESKTOP_XFCE );
		}
	}

	output_str = NULL;
	error_str = NULL;
	error = NULL;
	if( g_spawn_command_line_sync(
			"dbus-send --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.GetNameOwner string:org.gnome.SessionManager",
			&output_str, &error_str, &exit_status, &error )){
		ok = ( exit_status == 0 && output_str && strlen( output_str ) && ( !error_str || !strlen( error_str )));
		g_free( output_str );
		g_free( error_str );
		if( ok ){
			return( DESKTOP_GNOME );
		}
	}
	if( error ){
		g_warning( "%s: dbus-send: %s", thisfn, error->message );
		g_error_free( error );
	}

	output_str = NULL;
	error_str = NULL;
	error = NULL;
	if( g_spawn_command_line_sync(
			"xprop -root _DT_SAVE_MODE", &output_str, &error_str, &exit_status, &error )){
		ok = ( exit_status == 0 && output_str && strlen( output_str ) && ( !error_str || !strlen( error_str )));
		if( ok ){
			ok = ( g_strstr_len( output_str, -1, "xfce" ) != NULL );
		}
		g_free( output_str );
		g_free( error_str );
		if( ok ){
			return( DESKTOP_XFCE );
		}
	}
	if( error ){
		g_warning( "%s: xprop: %s", thisfn, error->message );
		g_error_free( error );
	}

	/* do not know how to identify ROX
	 * this one and other desktops are just identified as 'Old' (legacy systems)
	 */
	return( DESKTOP_OLD );
}

/*
 * fma_desktop_environment_get_label:
 * @id: desktop identifier.
 *
 * Returns: the label of the desktop environment.
 *
 * Defaults to returning the provided identifier if it is not found in
 * our internal reference.
 *
 * Since: 3.2
 */
const gchar *
fma_desktop_environment_get_label( const gchar *id )
{
	static const gchar *thisfn = "fma_desktop_environment_get_label";
	int i;

	g_return_val_if_fail( id && strlen( id ), NULL );

	for( i = 0 ; st_desktops[i].id ; ++ i ){
		if( !strcmp( st_desktops[i].id, id )){
			return( st_desktops[i].label );
		}
	}

	g_warning( "%s: unknown desktop identifier: %s", thisfn, id );

	return( id );
}
