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

#include <gio/gio.h>
#include <glib/gi18n.h>
#include <string.h>

#include "fma-gnome-vfs-uri.h"
#include "fma-selected-info.h"

/* private class data
 */
struct _FMASelectedInfoClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _FMASelectedInfoPrivate {
	gboolean       dispose_has_run;
	gchar         *uri;
	gchar         *filename;
	gchar         *dirname;
	gchar         *basename;
	gchar         *hostname;
	gchar         *username;
	gchar         *scheme;
	guint          port;
	gchar         *mimetype;
	GFileType      file_type;
	gboolean       can_read;
	gboolean       can_write;
	gboolean       can_execute;
	gchar         *owner;
	gboolean       attributes_are_set;
};


static GObjectClass *st_parent_class = NULL;

static GType            register_type( void );
static void             class_init( FMASelectedInfoClass *klass );
static void             instance_init( GTypeInstance *instance, gpointer klass );
static void             instance_dispose( GObject *object );
static void             instance_finalize( GObject *object );

static void             dump( const FMASelectedInfo *nsi );
static const char      *dump_file_type( GFileType type );
static FMASelectedInfo *new_from_uri( const gchar *uri, const gchar *mimetype, gchar **errmsg );
static void             query_file_attributes( FMASelectedInfo *info, GFile *location, gchar **errmsg );

GType
fma_selected_info_get_type( void )
{
	static GType object_type = 0;

	if( !object_type ){
		object_type = register_type();
	}

	return( object_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "fma_selected_info_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMASelectedInfoClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMASelectedInfo ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "FMASelectedInfo", &info, 0 );

	return( type );
}

static void
class_init( FMASelectedInfoClass *klass )
{
	static const gchar *thisfn = "fma_selected_info_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( FMASelectedInfoClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "fma_selected_info_instance_init";
	FMASelectedInfo *self;

	g_return_if_fail( FMA_IS_SELECTED_INFO( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = FMA_SELECTED_INFO( instance );

	self->private = g_new0( FMASelectedInfoPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->uri = NULL;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "fma_selected_info_instance_dispose";
	FMASelectedInfo *self;

	g_return_if_fail( FMA_IS_SELECTED_INFO( object ));
	self = FMA_SELECTED_INFO( object );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "fma_selected_info_instance_finalize";
	FMASelectedInfo *self;

	g_return_if_fail( FMA_IS_SELECTED_INFO( object ));

	self = FMA_SELECTED_INFO( object );

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_free( self->private->uri );
	g_free( self->private->filename );
	g_free( self->private->dirname );
	g_free( self->private->basename );
	g_free( self->private->hostname );
	g_free( self->private->username );
	g_free( self->private->scheme );
	g_free( self->private->mimetype );
	g_free( self->private->owner );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/*
 * fma_selected_info_copy_list:
 * @files: a #GList list of #FMASelectedInfo items.
 *
 * Returns: a copy of the provided @files list.
 */
GList *
fma_selected_info_copy_list( GList *files )
{
	GList *copy;
	GList *l;

	copy = g_list_copy( files );

	for( l = copy ; l != NULL ; l = l->next ){
		g_object_ref( G_OBJECT( l->data ));
	}

	return( copy );
}

/*
 * fma_selected_info_free_list:
 * @files: a #GList of #FMASelectedInfo items.
 *
 * Frees up the #GList @files.
 */
void
fma_selected_info_free_list( GList *files )
{
	g_list_foreach( files, ( GFunc ) g_object_unref, NULL );
	g_list_free( files );
}

/*
 * fma_selected_info_get_basename:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: the basename of the file associated with this
 * #FMASelectedInfo object, as a newly allocated string which
 * must be g_free() by the caller.
 */
gchar *
fma_selected_info_get_basename( const FMASelectedInfo *nsi )
{
	gchar *basename;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), NULL );

	basename = NULL;

	if( !nsi->private->dispose_has_run ){

		basename = g_strdup( nsi->private->basename );
	}

	return( basename );
}

/*
 * fma_selected_info_get_dirname:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: the dirname of the file associated with this
 * #FMASelectedInfo object, as a newly allocated string which
 * must be g_free() by the caller.
 */
gchar *
fma_selected_info_get_dirname( const FMASelectedInfo *nsi )
{
	gchar *dirname;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), NULL );

	dirname = NULL;

	if( !nsi->private->dispose_has_run ){

		dirname = g_strdup( nsi->private->dirname );
	}

	return( dirname );
}

/*
 * fma_selected_info_get_mime_type:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: the mime type associated with this #FMASelectedInfo object,
 * as a newly allocated string which should be g_free() by the caller.
 */
gchar *
fma_selected_info_get_mime_type( const FMASelectedInfo *nsi )
{
	gchar *mimetype;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), NULL );

	mimetype = NULL;

	if( !nsi->private->dispose_has_run ){

		if( nsi->private->mimetype ){
			mimetype = g_strdup( nsi->private->mimetype );
		}
	}

	return( mimetype );
}

/*
 * fma_selected_info_get_path:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: the filename of the item as a newly allocated string which
 * should be g_free() by the caller.
 */
gchar *
fma_selected_info_get_path( const FMASelectedInfo *nsi )
{
	gchar *path;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), NULL );

	path = NULL;

	if( !nsi->private->dispose_has_run ){

		path = g_strdup( nsi->private->filename );
	}

	return( path );
}

/*
 * fma_selected_info_get_uri:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: the URI associated with this #FMASelectedInfo object, as a
 * newly allocated string which should be g_free() by the caller.
 */
gchar *
fma_selected_info_get_uri( const FMASelectedInfo *nsi )
{
	gchar *uri;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), NULL );

	uri = NULL;

	if( !nsi->private->dispose_has_run ){

		uri = g_strdup( nsi->private->uri );
	}

	return( uri );
}

/*
 * fma_selected_info_get_uri_host:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: the host associated to this @nsi object, as a
 * newly allocated string which should be g_free() by the caller.
 */
gchar *
fma_selected_info_get_uri_host( const FMASelectedInfo *nsi )
{
	gchar *host;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), NULL );

	host = NULL;

	if( !nsi->private->dispose_has_run ){

		host = g_strdup( nsi->private->hostname );
	}

	return( host );
}

/*
 * fma_selected_info_get_uri_user:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: the user associated to this @nsi object, as a
 * newly allocated string which should be g_free() by the caller.
 */
gchar *
fma_selected_info_get_uri_user( const FMASelectedInfo *nsi )
{
	gchar *user;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), NULL );

	user = NULL;

	if( !nsi->private->dispose_has_run ){

		user = g_strdup( nsi->private->username );
	}

	return( user );
}

/*
 * fma_selected_info_get_uri_port:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: the port associated to this @nsi object.
 */
guint
fma_selected_info_get_uri_port( const FMASelectedInfo *nsi )
{
	guint port;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), 0 );

	port = 0;

	if( !nsi->private->dispose_has_run ){

		port = nsi->private->port;
	}

	return( port );
}

/*
 * fma_selected_info_get_uri_scheme:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: the scheme associated to this @nsi object, as a
 * newly allocated string which should be g_free() by the caller.
 */
gchar *
fma_selected_info_get_uri_scheme( const FMASelectedInfo *nsi )
{
	gchar *scheme;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), NULL );

	scheme = NULL;

	if( !nsi->private->dispose_has_run ){

		scheme = g_strdup( nsi->private->scheme );
	}

	return( scheme );
}

/*
 * fma_selected_info_is_directory:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: %TRUE if the item is a directory, %FALSE else.
 */
gboolean
fma_selected_info_is_directory( const FMASelectedInfo *nsi )
{
	gboolean is_dir;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), FALSE );

	is_dir = FALSE;

	if( !nsi->private->dispose_has_run ){

		is_dir = ( nsi->private->file_type == G_FILE_TYPE_DIRECTORY );
	}

	return( is_dir );
}

/*
 * fma_selected_info_is_regular:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: %TRUE if the item is a regular file, %FALSE else.
 */
gboolean
fma_selected_info_is_regular( const FMASelectedInfo *nsi )
{
	gboolean is_regular;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), FALSE );

	is_regular = FALSE;

	if( !nsi->private->dispose_has_run ){

		is_regular = ( nsi->private->file_type == G_FILE_TYPE_REGULAR );
	}

	return( is_regular );
}

/*
 * fma_selected_info_is_executable:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: %TRUE if the item is executable by the user, %FALSE else.
 */
gboolean
fma_selected_info_is_executable( const FMASelectedInfo *nsi )
{
	gboolean is_exe;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), FALSE );

	is_exe = FALSE;

	if( !nsi->private->dispose_has_run ){

		is_exe = nsi->private->can_execute;
	}

	return( is_exe );
}

/*
 * fma_selected_info_is_local:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: %TRUE if the item is on a local filesystem, %FALSE else.
 */
gboolean
fma_selected_info_is_local( const FMASelectedInfo *nsi )
{
	gboolean is_local;
	gchar *scheme;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), FALSE );

	is_local = FALSE;

	if( !nsi->private->dispose_has_run ){

		scheme = fma_selected_info_get_uri_scheme( nsi );
		is_local = ( strcmp( scheme, "file" ) == 0 );
		g_free( scheme );
	}

	return( is_local );
}

/*
 * fma_selected_info_is_owner:
 * @nsi: this #FMASelectedInfo object.
 * @user: the user to be tested against the owner of the @nsi object.
 *
 * Returns: %TRUE if the item is a owner, %FALSE else.
 */
gboolean
fma_selected_info_is_owner( const FMASelectedInfo *nsi, const gchar *user )
{
	gboolean is_owner;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), FALSE );

	is_owner = FALSE;

	if( !nsi->private->dispose_has_run ){

		is_owner = ( strcmp( nsi->private->owner, user ) == 0 );
	}

	return( is_owner );
}

/*
 * fma_selected_info_is_readable:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: %TRUE if the item is a readable, %FALSE else.
 */
gboolean
fma_selected_info_is_readable( const FMASelectedInfo *nsi )
{
	gboolean is_readable;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), FALSE );

	is_readable = FALSE;

	if( !nsi->private->dispose_has_run ){

		is_readable = nsi->private->can_read;
	}

	return( is_readable );
}

/*
 * fma_selected_info_is_writable:
 * @nsi: this #FMASelectedInfo object.
 *
 * Returns: %TRUE if the item is a writable, %FALSE else.
 */
gboolean
fma_selected_info_is_writable( const FMASelectedInfo *nsi )
{
	gboolean is_writable;

	g_return_val_if_fail( FMA_IS_SELECTED_INFO( nsi ), FALSE );

	is_writable = FALSE;

	if( !nsi->private->dispose_has_run ){

		is_writable = nsi->private->can_write;
	}

	return( is_writable );
}

/*
 * fma_selected_info_create_for_uri:
 * @uri: an URI.
 * @mimetype: the corresponding mime type, or %NULL.
 * @errmsg: a pointer to a string which will contain an error message on
 *  return.
 *
 * Returns: a newly allocated #FMASelectedInfo object for the given @uri.
 */
FMASelectedInfo *
fma_selected_info_create_for_uri( const gchar *uri, const gchar *mimetype, gchar **errmsg )
{
	static const gchar *thisfn = "fma_selected_info_create_for_uri";

	g_debug( "%s: uri=%s, mimetype=%s", thisfn, uri, mimetype );

	FMASelectedInfo *obj = new_from_uri( uri, mimetype, errmsg );

	return( obj );
}

static void
dump( const FMASelectedInfo *nsi )
{
	static const gchar *thisfn = "fma_selected_info_dump";

	g_debug( "%s:                uri=%s", thisfn, nsi->private->uri );
	g_debug( "%s:           mimetype=%s", thisfn, nsi->private->mimetype );
	g_debug( "%s:           filename=%s", thisfn, nsi->private->filename );
	g_debug( "%s:            dirname=%s", thisfn, nsi->private->dirname );
	g_debug( "%s:           basename=%s", thisfn, nsi->private->basename );
	g_debug( "%s:           hostname=%s", thisfn, nsi->private->hostname );
	g_debug( "%s:           username=%s", thisfn, nsi->private->username );
	g_debug( "%s:             scheme=%s", thisfn, nsi->private->scheme );
	g_debug( "%s:               port=%d", thisfn, nsi->private->port );
	g_debug( "%s: attributes_are_set=%s", thisfn, nsi->private->attributes_are_set ? "True":"False" );
	g_debug( "%s:          file_type=%s", thisfn, dump_file_type( nsi->private->file_type ));
	g_debug( "%s:           can_read=%s", thisfn, nsi->private->can_read ? "True":"False" );
	g_debug( "%s:          can_write=%s", thisfn, nsi->private->can_write ? "True":"False" );
	g_debug( "%s:        can_execute=%s", thisfn, nsi->private->can_execute ? "True":"False" );
	g_debug( "%s:              owner=%s", thisfn, nsi->private->owner );
}

static const char *
dump_file_type( GFileType type )
{
	switch( type ){
		case G_FILE_TYPE_REGULAR:
			return( "regular" );
		case G_FILE_TYPE_DIRECTORY:
			return( "directory" );
		case G_FILE_TYPE_SYMBOLIC_LINK:
			return( "symbolic link" );
		case G_FILE_TYPE_SPECIAL:
			return( "special (socket, fifo, blockdev, chardev)" );
		case G_FILE_TYPE_SHORTCUT:
			return( "shortcut" );
		case G_FILE_TYPE_MOUNTABLE:
			return( "mountable" );
		default:
			break;
	}
	return( "unknown" );
}

/*
 * Nautilus uses to address the desktop via the 'x-nautilus-desktop:///' URI.
 * g_filename_from_uri() complains that
 * "The URI 'x-nautilus-desktop:///' is not an absolute URI using the "file" scheme".
 * In this case, we prefer the vfs->path member wich is just a decomposition of the
 * URI, and does not try to interpret it.
 *
 * *********************************************************************************
 * Extract from RFC 2396:
 *
 * 2.4.3. Excluded US-ASCII Characters
 *
 * Although they are disallowed within the URI syntax, we include here a
 * description of those US-ASCII characters that have been excluded and
 * the reasons for their exclusion.
 *
 * The control characters in the US-ASCII coded character set are not
 * used within a URI, both because they are non-printable and because
 * they are likely to be misinterpreted by some control mechanisms.
 *
 * control = <US-ASCII coded characters 00-1F and 7F hexadecimal>
 *
 * The space character is excluded because significant spaces may
 * disappear and insignificant spaces may be introduced when URI are
 * transcribed or typeset or subjected to the treatment of word-
 * processing programs. Whitespace is also used to delimit URI in many
 * contexts.
 *
 * space = <US-ASCII coded character 20 hexadecimal>
 *
 * The angle-bracket "<" and ">" and double-quote (") characters are
 * excluded because they are often used as the delimiters around URI in
 * text documents and protocol fields. The character "#" is excluded
 * because it is used to delimit a URI from a fragment identifier in URI
 * references (Section 4). The percent character "%" is excluded because
 * it is used for the encoding of escaped characters.
 *
 * delims = "<" | ">" | "#" | "%" | <">
 *
 * Other characters are excluded because gateways and other transport
 * agents are known to sometimes modify such characters, or they are
 * used as delimiters.
 *
 * unwise = "{" | "}" | "|" | "\" | "^" | "[" | "]" | "`"
 *
 * Data corresponding to excluded characters must be escaped in order to
 * be properly represented within a URI.
 *
 * pwi 2011-01-04:
 * It results from the above excerpt that:
 * - as double quotes are not valid character in URI, they have to be
 *   escaped as %22, and so Nautilus does
 * - but simple quotes are not forbidden, and so have not to be
 *   escaped, and so Nautilus does not escape them
 *
 * As a result, we may have valid, non-escaped, simple quotes in an URI.
 */
static FMASelectedInfo *
new_from_uri( const gchar *uri, const gchar *mimetype, gchar **errmsg )
{
	GFile *location;
	FMAGnomeVFSURI *vfs;

	FMASelectedInfo *info = g_object_new( FMA_TYPE_SELECTED_INFO, NULL );

	info->private->uri = g_strdup( uri );
	if( mimetype ){
		info->private->mimetype = g_strdup( mimetype );
	}

	/* pwi 2011-05-18
	 * Filename and dirname should be taken from the GFile location, itself taken
	 * from the URI, so that we have dir='/home/pierre/.gvfs/sftp on stormy.trychlos.org/etc'
	 * Taking filename and dirname from URI just gives '/etc'
	 * see #650523
	 */
	location = g_file_new_for_uri( uri );
	info->private->filename = g_file_get_path( location );

	vfs = g_new0( FMAGnomeVFSURI, 1 );
	fma_gnome_vfs_uri_parse( vfs, uri );
	if( !info->private->filename ){
		g_debug( "fma_selected_info_new_from_uri: uri='%s', filename=NULL, setting it to '%s'", uri, vfs->path );
		info->private->filename = g_strdup( vfs->path );
	}

	info->private->basename = g_path_get_basename( info->private->filename );
	info->private->dirname = g_path_get_dirname( info->private->filename );
	info->private->hostname = g_strdup( vfs->host_name );
	info->private->username = g_strdup( vfs->user_name );
	info->private->scheme = g_strdup( vfs->scheme );
	info->private->port = vfs->host_port;
	fma_gnome_vfs_uri_free( vfs );

	query_file_attributes( info, location, errmsg );
	g_object_unref( location );

	dump( info );

	return( info );
}

static void
query_file_attributes( FMASelectedInfo *nsi, GFile *location, gchar **errmsg )
{
	static const gchar *thisfn = "fma_selected_info_query_file_attributes";
	GError *error;

	error = NULL;
	GFileInfo *info = g_file_query_info( location,
			G_FILE_ATTRIBUTE_STANDARD_TYPE
				"," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE
				"," G_FILE_ATTRIBUTE_ACCESS_CAN_READ
				"," G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE
				"," G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE
				"," G_FILE_ATTRIBUTE_OWNER_USER,
			G_FILE_QUERY_INFO_NONE, NULL, &error );

	if( error ){
		if( errmsg ){
			*errmsg = g_strdup_printf( _( "Error when querying information for %s URI: %s" ), nsi->private->uri, error->message );
		} else {
			g_warning( "%s: uri=%s, g_file_query_info: %s", thisfn, nsi->private->uri, error->message );
		}
		g_error_free( error );
		return;
	}

	if( !nsi->private->mimetype ){
		nsi->private->mimetype = g_strdup( g_file_info_get_attribute_as_string( info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ));
	}

	nsi->private->file_type = ( GFileType ) g_file_info_get_attribute_uint32( info, G_FILE_ATTRIBUTE_STANDARD_TYPE );

	nsi->private->can_read = g_file_info_get_attribute_boolean( info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ );
	nsi->private->can_write = g_file_info_get_attribute_boolean( info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE );
	nsi->private->can_execute = g_file_info_get_attribute_boolean( info, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE );

	nsi->private->owner = g_strdup( g_file_info_get_attribute_as_string( info, G_FILE_ATTRIBUTE_OWNER_USER ));

	nsi->private->attributes_are_set = TRUE;

	g_object_unref( info );
}
