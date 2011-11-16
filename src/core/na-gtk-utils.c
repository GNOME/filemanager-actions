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

#include <string.h>

#include "na-gtk-utils.h"

/*
 * na_gtk_utils_search_for_child_widget:
 * @container: a #GtkContainer, usually the #GtkWindow toplevel.
 * @name: the name of the searched widget.
 *
 * Returns: the searched widget.
 */
GtkWidget *
na_gtk_utils_search_for_child_widget( GtkContainer *container, const gchar *name )
{
	GList *children = gtk_container_get_children( container );
	GList *ic;
	GtkWidget *found = NULL;
	GtkWidget *child;
	const gchar *child_name;

	for( ic = children ; ic && !found ; ic = ic->next ){

		if( GTK_IS_WIDGET( ic->data )){
			child = GTK_WIDGET( ic->data );
#if GTK_CHECK_VERSION( 2, 20, 0 )
			child_name = gtk_buildable_get_name( GTK_BUILDABLE( child ));
#else
			child_name = gtk_widget_get_name( child );
#endif
			if( child_name && strlen( child_name )){
				/*g_debug( "%s: child=%s", thisfn, child_name );*/

				if( !g_ascii_strcasecmp( name, child_name )){
					found = child;
					break;

				} else if( GTK_IS_CONTAINER( child )){
					found = na_gtk_utils_search_for_child_widget( GTK_CONTAINER( child ), name );
				}
			}
		}
	}

	g_list_free( children );
	return( found );
}

#ifdef NA_MAINTAINER_MODE
static void
dump_children( GtkContainer *container, int level )
{
	static const gchar *thisfn = "na_gtk_utils_dump_children";
	GList *children = gtk_container_get_children( container );
	GList *ic;
	GtkWidget *child;
	const gchar *child_name;
	GString *prefix;
	int i;

	prefix = g_string_new( "" );
	for( i = 0 ; i < level ; ++i ){
		g_string_append_printf( prefix, "%s", "|  " );
	}

	for( ic = children ; ic ; ic = ic->next ){

		if( GTK_IS_WIDGET( ic->data )){
			child = GTK_WIDGET( ic->data );
#if GTK_CHECK_VERSION( 2, 20, 0 )
			child_name = gtk_buildable_get_name( GTK_BUILDABLE( child ));
#else
			child_name = gtk_widget_get_name( child );
#endif
			if( child_name && strlen( child_name )){
				g_debug( "%s: %s%s\t%s", thisfn, prefix->str, G_OBJECT_TYPE_NAME( child ), child_name );

				if( GTK_IS_CONTAINER( child )){
					dump_children( GTK_CONTAINER( child ), level+1 );
				}
			}
		}
	}

	g_list_free( children );
	g_string_free( prefix, TRUE );
}

void
na_gtk_utils_dump_children( GtkContainer *container )
{
	static const gchar *thisfn = "na_gtk_utils_dump_children";

	g_debug( "%s: container=%p", thisfn, container );

	dump_children( container, 0 );
}
#endif
