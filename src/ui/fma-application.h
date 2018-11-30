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

#ifndef __UI_FMA_APPLICATION_H__
#define __UI_FMA_APPLICATION_H__

/**
 * SECTION: fma_application
 * @short_description: #FMAApplication class definition.
 * @include: ui/fma-application.h
 *
 * This is the main class for filemanager-actions-config-tool program.
 *
 * The #FMAApplication object is instantiated from main() function,
 * then later #g_object_unref() after fma_application_run() has
 * returned.
 *
 * FMAApplication is a non-unique application e.g. the user is able
 * to run several instance of the applications, all pointing to the
 * same actions set, potentially being able to simultaneously have
 * different views or to simultaneously act on different subsets.
 */

#include <gtk/gtk.h>

#include "core/fma-updater.h"

G_BEGIN_DECLS

#define FMA_TYPE_APPLICATION                ( fma_application_get_type())
#define FMA_APPLICATION( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_APPLICATION, FMAApplication ))
#define FMA_APPLICATION_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_APPLICATION, FMAApplicationClass ))
#define FMA_IS_APPLICATION( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_APPLICATION ))
#define FMA_IS_APPLICATION_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_APPLICATION ))
#define FMA_APPLICATION_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_APPLICATION, FMAApplicationClass ))

typedef struct _FMAApplicationPrivate       FMAApplicationPrivate;

typedef struct {
	/*< private >*/
	GtkApplicationClass    parent;
}
	FMAApplicationClass;

typedef struct {
	/*< private >*/
	GtkApplication         parent;
	FMAApplicationPrivate *private;
}
	FMAApplication;

/**
 * FMAExitCode:
 *
 * The code returned by the application.
 *
 * @FMA_EXIT_CODE_PROGRAM = -1: this is a program error code.
 * @FMA_EXIT_CODE_OK = 0:       the program has successfully run, and returns zero.
 * @FMA_EXIT_CODE_ARGS = 1:     unable to interpret command-line options
 * @FMA_EXIT_CODE_WINDOW = 2:   unable to create a window
 */
typedef enum {
	FMA_EXIT_CODE_PROGRAM = -1,
	FMA_EXIT_CODE_OK = 0,
	FMA_EXIT_CODE_ARGS,
	FMA_EXIT_CODE_WINDOW,
}
	FMAExitCode;

GType           fma_application_get_type            ( void );

FMAApplication *fma_application_new                 ( void );

int             fma_application_run_with_args       ( FMAApplication *application,
																int argc,
																GStrv argv );

gchar          *fma_application_get_application_name( const FMAApplication *application );

FMAUpdater     *fma_application_get_updater         ( const FMAApplication *application );

GtkWindow      *fma_application_get_main_window     ( FMAApplication *application );

G_END_DECLS

#endif /* __UI_FMA_APPLICATION_H__ */
