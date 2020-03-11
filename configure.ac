# FileManager-Actions
# A file-manager extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009-2014 Pierre Wieser and others (see AUTHORS)
#
# FileManager-Actions is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# FileManager-Actions is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with FileManager-Actions; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.
#
# Authors:
#   Frederic Ruaudel <grumz@grumz.net>
#   Rodrigo Moya <rodrigo@gnome-db.org>
#   Pierre Wieser <pwieser@trychlos.org>
#   ... and many others (see AUTHORS)

AC_PREREQ([2.53])

AC_INIT([FileManager-Actions],[3.4.1],[maintainer@nautilus-actions.org],,[http://www.nautilus-actions.org])
AX_IS_RELEASE([git-directory])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE

# Code coverage
AX_CODE_COVERAGE()

AC_CONFIG_SRCDIR([src/ui/main.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

# check for compiler characteristics and options
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_STDC_HEADERS
AM_DISABLE_STATIC

# define specific compilation options
FMA_COMPILER_WARNINGS([],[-ansi -Wno-overlength-strings -Wformat=2])
FMA_COMPILER_LINK_AS_NEEDED
AC_SUBST([AM_CFLAGS],["${AM_CFLAGS} ${WARN_CFLAGS}"])

# other traditional tools
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Maintainer mode
FMA_MAINTAINER_CHECK_MODE

# libtool
AM_PROG_LIBTOOL

# we are using pkgconfig for all development libraries we need
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "${PKG_CONFIG}" = "no"; then
	AC_MSG_ERROR([You need to install pkg-config])
fi

# required versions of tools and libraries
source ${0%/*}/fma_required_versions

# localization
GETTEXT_PACKAGE=${PACKAGE}
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["${GETTEXT_PACKAGE}"],[gettext package])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([${intltool_required}])

# a counter of fatal warnings emitted
let fma_fatal_count=0
fma_fatal_list=""

# GLib marshaling
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal, no)
if test "${GLIB_GENMARSHAL}" = "no"; then
	AC_MSG_WARN([glib2-devel installed, but glib-genmarshal not found])
	let fma_fatal_count+=1
fi

# add --enable-docs (for all documentation stuff)
# have to be set early in the configure.ac in order to set all needed
# variables
FMA_ENABLE_DOCS

# Have and want GConf support ?
# As of 3.4 release, we are deprecating GConf, disabling it by default.
# We only search for the libraries when explicitely required from 
# configure. This is a first step deprecation; in a second step, we
# will emit a warning.
FMA_CHECK_FOR_GCONF

# cf. fma_required_versions
# aclocal complains if the comma is not just after the square bracket
FMA_CHECK_MODULE([GTK3],    [gtk+-3.0],   [${gtk_required}])
FMA_CHECK_MODULE([GLIB],    [glib-2.0],   [${glib_required}])
FMA_CHECK_MODULE([GMODULE], [gmodule-2.0],[${glib_required}])
FMA_CHECK_MODULE([GIO_UNIX],[gio-unix-2.0])
FMA_CHECK_MODULE([GTOP],    [libgtop-2.0],[${gtop_required}])
FMA_CHECK_MODULE([LIBXML],  [libxml-2.0], [${xml_required}])
FMA_CHECK_MODULE([UUID],    [uuid],       [${uuid_required}])

AC_SUBST([NAUTILUS_ACTIONS_CFLAGS])
AC_SUBST([NAUTILUS_ACTIONS_LIBS])

# print the debug output be printed to the console
AC_DEFINE([NAUTILUS_ACTIONS_DEBUG],["NAUTILUS_ACTIONS_DEBUG"],[Debug environment variable])

# target a file manager (nautilus, nemo, caja, ...)
FMA_TARGET_FILE_MANAGER

# add --with-default-io-provider option
FMA_SET_DEFAULT_IO_PROVIDER([io-desktop])

# Enable deprecated functions ?
FMA_MAINTAINER_CHECK_FOR_DEPRECATED

# check for gtk-doc
# - starting with gtk-doc 1.16, is able to handle srcdir != builddir
# - API documentation should not be built (would be incomplete) when deprecated
#   symbols are not enabled
GTK_DOC_CHECK([1.16],[--flavour no-tmpl])
if test "${enable_gtk_doc}" = "yes" -a "${enable_deprecated}" = "no"; then
	AC_MSG_WARN([API documentation will be incomplete as deprecated symbols are disabled])
fi

# check for yelp-tools
YELP_HELP_INIT

###

AC_CONFIG_FILES([
	Makefile
	data/Makefile
	data/actions/Makefile
	data/exports/Makefile
	data/gconf-schemas/Makefile
	docs/Makefile
	docs/manual/Makefile
	docs/reference/Makefile
	docs/reference/version.xml
	icons/Makefile
	icons/16x16/Makefile
	icons/22x22/Makefile
	icons/32x32/Makefile
	icons/48x48/Makefile
	icons/scalable/Makefile
	m4/Makefile
	maintainer/Makefile
	po/Makefile.in
	src/Makefile
	src/api/Makefile
	src/core/Makefile
	src/io-desktop/Makefile
	src/io-gconf/Makefile
	src/io-xml/Makefile
	src/plugin-menu/Makefile
	src/plugin-tracker/Makefile
	src/test/Makefile
	src/ui/Makefile
	src/utils/Makefile
])

# display and keep configuration informations
config_options="`$as_echo "$ac_configure_args" | sed 's/^ //; s/[\\""\`\$]/\\\\&/g'`" 
AC_DEFINE_UNQUOTED([FMA_CONFIG_OPTIONS],["$0 ${config_options}"],["Configure options"])

AC_OUTPUT

msg_gtk_doc=`if test "x${enable_gtk_doc}" = "xno"; then echo "disabled"; else echo "enabled"; fi`
$as_echo "
	${PACKAGE_STRING} configuration summary:

	Installation prefix             ${prefix}
	Build system type               ${ac_cv_build}
	Gtk+ version                    ${GTK3_msg_version}
	GLib version                    ${GLIB_msg_version}
	Default I/O provider            ${with_default_io_provider}"
if test "${with_nautilus_ok}" = "yes"; then
	$as_echo "	Targeting Nautilus
	  installing extensions in      ${with_nautilus_extdir}"
fi
if test "${with_nemo_ok}" = "yes"; then
	$as_echo "	Targeting Nemo
	  installing extensions in      ${with_nemo_extdir}"
fi
if test "${with_caja_ok}" = "yes"; then
	$as_echo "	Targeting Caja
	  installing extensions in      ${with_caja_extdir}"
fi
$as_echo "	GConf enabled                   ${enable_gconf}"
if test "${enable_gconf}" = "yes"; then
	$as_echo "	GConf schemas installation      ${msg_schemas_install}"
fi
$as_echo "	Maintainer mode                 ${msg_maintainer_mode}
	Enable deprecated functions     ${enable_deprecated}
	API Reference generation        ${msg_gtk_doc}
	HTML User's Manuals generation  ${msg_html_manuals}
	PDF User's Manuals generation   ${msg_pdf_manuals}
"
if test ${fma_fatal_count} -gt 0; then
	$as_echo "${fma_fatal_list}
"
	exit 1
fi

#	Preprocessor flags              ${AM_CPPFLAGS}
#	Compiler flags                  ${AM_CFLAGS} ${NAUTILUS_ACTIONS_CFLAGS}
#	Linker flags                    ${NAUTILUS_ACTIONS_LIBS}
