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

#ifndef __IO_XML_FMA_XML_KEYS_H__
#define __IO_XML_FMA_XML_KEYS_H__

#include <api/fma-data-def.h>

G_BEGIN_DECLS

/* XML element names (GConf schema)
 * used in GCONF_SCHEMA_V1 and GCONF_SCHEMA_V2
 *
 * Up to v 1.10, export used to contain a full schema description,
 * while import only checked for applyto keys (along with locale
 * and default)
 *
 * Starting with 1.11, we have introduced a lighter export schema
 * (without owner and short and long descriptions)
 */
#define FMA_XML_KEY_SCHEMA_ROOT					"gconfschemafile"
#define FMA_XML_KEY_SCHEMA_LIST					"schemalist"
#define FMA_XML_KEY_SCHEMA_NODE					"schema"

#define FMA_XML_KEY_SCHEMA_NODE_KEY				"key"
#define FMA_XML_KEY_SCHEMA_NODE_APPLYTO			"applyto"
#define FMA_XML_KEY_SCHEMA_NODE_OWNER			"owner"			/* v1 only */
#define FMA_XML_KEY_SCHEMA_NODE_TYPE			"type"
#define FMA_XML_KEY_SCHEMA_NODE_LISTTYPE		"list_type"
#define FMA_XML_KEY_SCHEMA_NODE_LOCALE			"locale"
#define FMA_XML_KEY_SCHEMA_NODE_DEFAULT			"default"

#define FMA_XML_KEY_SCHEMA_NODE_LOCALE_DEFAULT	"default"
#define FMA_XML_KEY_SCHEMA_NODE_LOCALE_SHORT	"short"			/* v1 only */
#define FMA_XML_KEY_SCHEMA_NODE_LOCALE_LONG		"long"			/* v1 only */

/* this structure is statically allocated (cf. fma-xml-keys.c)
 * and let us check the validity of each element node
 */
typedef struct {
	gchar   *key;						/* static data */
	gboolean v1;
	gboolean v2;

	gboolean reader_found;				/* dynamic data */
}
	FMAXMLKeyStr;

/* XML element names (GConf dump)
 * used in FORMAT_GCONF_ENTRY
 */
#define FMA_XML_KEY_DUMP_ROOT						"gconfentryfile"
#define FMA_XML_KEY_DUMP_LIST						"entrylist"
#define FMA_XML_KEY_DUMP_NODE						"entry"

#define FMA_XML_KEY_DUMP_LIST_PARM_BASE				"base"

#define FMA_XML_KEY_DUMP_NODE_KEY					"key"
#define FMA_XML_KEY_DUMP_NODE_VALUE					"value"

#define FMA_XML_KEY_DUMP_NODE_VALUE_TYPE_STRING		"string"
#define FMA_XML_KEY_DUMP_NODE_VALUE_LIST			"list"
#define FMA_XML_KEY_DUMP_NODE_VALUE_LIST_PARM_TYPE	"type"

G_END_DECLS

#endif /* __IO_XML_FMA_XML_KEYS_H__ */
