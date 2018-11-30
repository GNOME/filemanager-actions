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
#include <libintl.h>

#include "api/fma-core-utils.h"

#include "core/fma-about.h"

#include "fma-application.h"
#include "fma-main-window.h"
#include "fma-menu.h"

/* private instance data
 */
struct _FMAApplicationPrivate {
	gboolean        dispose_has_run;

	const gchar    *application_name;	/* new: st_application_name localized version */
	const gchar    *description;		/* new: st_description localized version */
	const gchar    *icon_name;			/* new: icon name */
	int             argc;
	GStrv           argv;
	int             code;

	FMAUpdater      *updater;
	FMAMainWindow   *main_window;
};

static const gchar *st_application_name	= N_( "FileManager-Actions Configuration Tool" );
static const gchar *st_description		= N_( "A user interface to edit your own contextual actions" );
static const gchar *st_application_id   = "org.gnome.filemanager-actions.ConfigurationTool";

static gboolean     st_non_unique_opt = FALSE;
static gboolean     st_version_opt    = FALSE;

static GOptionEntry st_option_entries[] = {
	{ "non-unique", 'n', 0, G_OPTION_ARG_NONE, &st_non_unique_opt,
			N_( "Set it to run multiple instances of the program [unique]" ), NULL },
	{ "version"   , 'v', 0, G_OPTION_ARG_NONE, &st_version_opt,
			N_( "Output the version number, and exit gracefully [no]" ), NULL },
	{ NULL }
};

static GtkApplicationClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( FMAApplicationClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *application );
static void     instance_finalize( GObject *application );
static void     init_i18n( FMAApplication *application );
static gboolean init_gtk_args( FMAApplication *application );
static gboolean manage_options( FMAApplication *application );
static void     application_startup( GApplication *application );
static void     application_activate( GApplication *application );
static void     application_open( GApplication *application, GFile **files, gint n_files, const gchar *hint );

GType
fma_application_get_type( void )
{
	static GType application_type = 0;

	if( !application_type ){
		application_type = register_type();
	}

	return( application_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "fma_application_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMAApplicationClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMAApplication ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( GTK_TYPE_APPLICATION, "FMAApplication", &info, 0 );

	return( type );
}

static void
class_init( FMAApplicationClass *klass )
{
	static const gchar *thisfn = "fma_application_class_init";

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = GTK_APPLICATION_CLASS( g_type_class_peek_parent( klass ));

	G_OBJECT_CLASS( klass )->dispose = instance_dispose;
	G_OBJECT_CLASS( klass )->finalize = instance_finalize;

	G_APPLICATION_CLASS( klass )->startup = application_startup;
	G_APPLICATION_CLASS( klass )->activate = application_activate;
	G_APPLICATION_CLASS( klass )->open = application_open;
}

static void
instance_init( GTypeInstance *application, gpointer klass )
{
	static const gchar *thisfn = "fma_application_instance_init";
	FMAApplication *self;

	g_return_if_fail( FMA_IS_APPLICATION( application ));

	g_debug( "%s: application=%p (%s), klass=%p",
			thisfn, ( void * ) application, G_OBJECT_TYPE_NAME( application ), ( void * ) klass );

	self = FMA_APPLICATION( application );

	self->private = g_new0( FMAApplicationPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *application )
{
	static const gchar *thisfn = "fma_application_instance_dispose";
	FMAApplicationPrivate *priv;

	g_return_if_fail( application && FMA_IS_APPLICATION( application ));

	priv = FMA_APPLICATION( application )->private;

	if( !priv->dispose_has_run ){

		g_debug( "%s: application=%p (%s)", thisfn, ( void * ) application, G_OBJECT_TYPE_NAME( application ));

		priv->dispose_has_run = TRUE;

		if( priv->updater ){
			g_clear_object( &priv->updater );
		}
	}

	/* chain up to the parent class */
	G_OBJECT_CLASS( st_parent_class )->dispose( application );
}

static void
instance_finalize( GObject *application )
{
	static const gchar *thisfn = "fma_application_instance_finalize";
	FMAApplication *self;

	g_return_if_fail( FMA_IS_APPLICATION( application ));

	g_debug( "%s: application=%p (%s)", thisfn, ( void * ) application, G_OBJECT_TYPE_NAME( application ));

	self = FMA_APPLICATION( application );

	g_free( self->private );

	/* chain call to the parent class */
	G_OBJECT_CLASS( st_parent_class )->finalize( application );
}

/**
 * fma_application_new:
 *
 * Returns: a newly allocated FMAApplication object.
 */
FMAApplication *
fma_application_new( void )
{
	FMAApplication *application;
	FMAApplicationPrivate *priv;

	application = g_object_new( FMA_TYPE_APPLICATION,
			"application-id", st_application_id,
			NULL );

	priv = application->private;
	priv->application_name = gettext( st_application_name );
	priv->description = gettext( st_description );
	priv->icon_name = fma_about_get_icon_name();

	return( application );
}

/**
 * fma_application_run_with_args:
 * @application: this #GtkApplication -derived instance.
 * @argc:
 * @argv:
 *
 * Starts and runs the application.
 * Takes care of creating, initializing, and running the main window.
 *
 * All steps are implemented as virtual functions which provide some
 * suitable defaults, and may be overridden by a derived class.
 *
 * Returns: an %int code suitable as an exit code for the program.
 *
 * Though it is defined as a virtual function itself, it should be very
 * seldomly needed to override this in a derived class.
 */
int
fma_application_run_with_args( FMAApplication *application, int argc, GStrv argv )
{
	static const gchar *thisfn = "fma_application_run_with_args";
	FMAApplicationPrivate *priv;

	g_debug( "%s: application=%p (%s), argc=%d",
			thisfn,
			( void * ) application, G_OBJECT_TYPE_NAME( application ),
			argc );

	g_return_val_if_fail( application && FMA_IS_APPLICATION( application ), FMA_EXIT_CODE_PROGRAM );

	priv = application->private;

	if( !priv->dispose_has_run ){

		priv->argc = argc;
		priv->argv = g_strdupv( argv );
		priv->code = FMA_EXIT_CODE_OK;

		init_i18n( application );
		g_set_application_name( priv->application_name );
		gtk_window_set_default_icon_name( priv->icon_name );

		if( init_gtk_args( application ) &&
			manage_options( application )){

			g_debug( "%s: entering g_application_run", thisfn );
			priv->code = g_application_run( G_APPLICATION( application ), 0, NULL );
		}
	}

	return( priv->code );
}

/*
 * i18n initialization
 *
 * Returns: %TRUE to continue the execution, %FALSE to terminate the program.
 * The program exit code will be taken from @code.
 */
static void
init_i18n( FMAApplication *application )
{
	static const gchar *thisfn = "fma_application_init_i18n";

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

#ifdef ENABLE_NLS
	bindtextdomain( GETTEXT_PACKAGE, GNOMELOCALEDIR );
# ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" );
# endif
	textdomain( GETTEXT_PACKAGE );
#endif
}

/*
 * Pre-Gtk+ initialization
 *
 * Though GApplication has its own infrastructure to handle command-line
 * arguments, it appears that it does not deal with Gtk+-specific arguments.
 * We so have to explicitely call gtk_init_with_args() in order to let Gtk+
 * "eat" its own arguments, and only have to handle our owns...
 */
static gboolean
init_gtk_args( FMAApplication *application )
{
	static const gchar *thisfn = "fma_application_init_gtk_args";
	FMAApplicationPrivate *priv;
	gboolean ret;
	char *parameter_string;
	GError *error;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	priv = application->private;

	parameter_string = g_strdup( g_get_application_name());
	error = NULL;
	ret = gtk_init_with_args(
			&priv->argc,
			( char *** ) &priv->argv,
			parameter_string,
			st_option_entries,
			GETTEXT_PACKAGE,
			&error );
	if( !ret ){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
		ret = FALSE;
		priv->code = FMA_EXIT_CODE_ARGS;
	}
	g_free( parameter_string );

	return( ret );
}

static gboolean
manage_options( FMAApplication *application )
{
	static const gchar *thisfn = "fma_application_manage_options";
	gboolean ret;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	ret = TRUE;

	/* display the program version ?
	 * if yes, then stops here, exiting with code ok
	 */
	if( st_version_opt ){
		fma_core_utils_print_version();
		ret = FALSE;
	}

	/* run the application as non-unique ?
	 */
	if( ret && st_non_unique_opt ){
		g_application_set_flags( G_APPLICATION( application ), G_APPLICATION_NON_UNIQUE );
	}

	return( ret );
}

/*
 * https://wiki.gnome.org/HowDoI/GtkApplication
 *
 * Invoked on the primary instance immediately after registration.
 *
 * When your application starts, the startup signal will be fired. This
 * gives you a chance to perform initialisation tasks that are not
 * directly related to showing a new window. After this, depending on
 * how the application is started, either activate or open will be called
 * next.
 *
 * GtkApplication defaults to applications being single-instance. If the
 * user attempts to start a second instance of a single-instance
 * application then GtkApplication will signal the first instance and
 * you will receive additional activate or open signals. In this case,
 * the second instance will exit immediately, without calling startup
 * or shutdown.
 *
 * For this reason, you should do essentially no work at all from main().
 * All startup initialisation should be done in startup. This avoids
 * wasting work in the second-instance case where the program just exits
 * immediately.
 */
static void
application_startup( GApplication *application )
{
	static const gchar *thisfn = "fma_application_startup";
	FMAApplicationPrivate *priv;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	g_return_if_fail( application && FMA_IS_APPLICATION( application ));
	priv = FMA_APPLICATION( application )->private;

	/* chain up to the parent class */
	if( G_APPLICATION_CLASS( st_parent_class )->startup ){
		G_APPLICATION_CLASS( st_parent_class )->startup( application );
	}

	/* create the FMAPivot object (loading the plugins and so on)
	 * after having dealt with command-line arguments
	 */
	priv->updater = fma_updater_new();
	fma_pivot_set_loadable( FMA_PIVOT( priv->updater ), PIVOT_LOAD_ALL );

	/* define the application menu */
	fma_menu_app( FMA_APPLICATION( application ));
}

/*
 * https://wiki.gnome.org/Projects/GLib/GApplication/Introduction
 * https://wiki.gnome.org/HowDoI/GtkApplication
 *
 * activate is executed by GApplication when the application is "activated".
 * This corresponds to the program being run from the command line, or when
 * its icon is clicked on in an application launcher.
 * From a semantic standpoint, activate should usually do one of two things,
 * depending on the type of application.
 *
 * If your application is the type of application that deals with several
 * documents at a time, in separate windows (and/or tabs) then activate
 * should involve showing a window or creating a tab for a new document.
 *
 * If your application is more like the type of application with one primary
 * main window then activate should usually involve raising this window with
 * gtk_window_present(). It is the choice of the application in this case if
 * the window itself is constructed in startup or on the first execution of
 * activate.
 *
 * activate is potentially called many times in a process or maybe never.
 * If the process is started without files to open then activate will be run
 * after startup. It may also be run again if a second instance of the
 * process is started.
 */
static void
application_activate( GApplication *application )
{
	static const gchar *thisfn = "fma_application_activate";
	FMAApplicationPrivate *priv;
	GList *windows_list;
	FMAMainWindow *main_window;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	g_return_if_fail( application && FMA_IS_APPLICATION( application ));

	priv = FMA_APPLICATION( application )->private;
	windows_list = gtk_application_get_windows( GTK_APPLICATION( application ));

	/* if the application is unique, have only one main window */
	if( !st_non_unique_opt ){
		if( !g_list_length( windows_list )){
			main_window = fma_main_window_new( FMA_APPLICATION( application ));
			g_debug( "%s: main window instantiated at %p", thisfn, main_window );
		} else {
			main_window = ( FMAMainWindow * ) windows_list->data;
		}

	/* have as many main windows we want */
	} else {
		main_window = fma_main_window_new( FMA_APPLICATION( application ));
	}

	g_return_if_fail( main_window && FMA_IS_MAIN_WINDOW( main_window ));

	priv->main_window = main_window;

	gtk_window_present( GTK_WINDOW( main_window ));
}

/*
 * https://wiki.gnome.org/Projects/GLib/GApplication/Introduction
 *
 * open is similar to activate, but it is used when some files have been
 * passed to the application to open.
 * In fact, you could think of activate as a special case of open: the
 * one with zero files.
 * Similar to activate, open should create a window or tab. It should
 * open the file in this window. If multiple files are given, possibly
 * several windows should be opened.
 * open will only be invoked in the case that your application declares
 * that it supports opening files with the G_APPLICATION_HANDLES_OPEN
 * GApplicationFlag.
 *
 * Openbook: as the G_APPLICATION_HANDLES_OPEN flag is not set, then
 * this function should never be called.
 */
static void
application_open( GApplication *application, GFile **files, gint n_files, const gchar *hint )
{
	static const gchar *thisfn = "fma_application_open";

	g_warning( "%s: application=%p, n_files=%d, hint=%s: unexpected run here",
			thisfn, ( void * ) application, n_files, hint );
}

/**
 * fma_application_get_application_name:
 * @application: this #FMAApplication instance.
 *
 * Returns: the application name as a newly allocated string which should
 * be be g_free() by the caller.
 */
gchar *
fma_application_get_application_name( const FMAApplication *application )
{
	FMAApplicationPrivate *priv;
	gchar *name = NULL;

	g_return_val_if_fail( application && FMA_IS_APPLICATION( application ), NULL );

	priv = application->private;

	if( !priv->dispose_has_run ){

		name = g_strdup( priv->application_name );
	}

	return( name );
}

/**
 * fma_application_get_updater:
 * @application: this FMAApplication object.
 *
 * Returns a pointer on the #FMAUpdater object.
 *
 * The returned pointer is owned by the #FMAApplication object.
 * It should not be g_free() not g_object_unref() by the caller.
 */
FMAUpdater *
fma_application_get_updater( const FMAApplication *application )
{
	FMAApplicationPrivate *priv;
	FMAUpdater *updater = NULL;

	g_return_val_if_fail( application && FMA_IS_APPLICATION( application ), NULL );

	priv = application->private;

	if( !priv->dispose_has_run ){

		updater = priv->updater;
	}

	return( updater );
}

/**
 * fma_application_get_main_window:
 * @application: this FMAApplication object.
 *
 * Returns: the main window.
 */
GtkWindow *
fma_application_get_main_window( FMAApplication *application )
{
	FMAApplicationPrivate *priv;

	g_return_val_if_fail( application && FMA_IS_APPLICATION( application ), NULL );

	priv = application->private;

	if( !priv->dispose_has_run ){

		return( GTK_WINDOW( priv->main_window ));
	}

	return( NULL );
}
