/* GLIB - Library of useful routines for C programming
 * Copyright (c) 2024  Florian "sp1rit" <sp1rit@disroot.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GLIB_ANDROID_PRIVATE_H__
#define __GLIB_ANDROID_PRIVATE_H__

#include "gmessages.h"

#include "glib-android.h"

void g_android_finalize (void);

gchar *g_android_get_files_dir          (void);
gchar *g_android_get_external_files_dir (void);
gchar *g_android_get_cache_dir          (void);

gchar *g_android_get_package_name  (void);
gchar *g_android_get_package_label (void);

void g_android_print_handler                      (const char *message);
void g_android_printerr_handler                   (const char *message);
void g_android_log_handler                        (const gchar *log_domain,
                                                   GLogLevelFlags log_level,
                                                   const char *message,
                                                   gpointer user_data);
GLogWriterOutput g_android_structured_log_handler (GLogLevelFlags log_level,
                                                   const GLogField *fields,
                                                   gsize n_fields,
                                                   gpointer user_data);

#endif /* __GLIB_ANDROID_PRIVATE_H__ */
