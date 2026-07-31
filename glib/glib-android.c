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

#include <android/log.h>

#include "gtestutils.h"
#include "gmessages.h"

#include "glib-androidprivate.h"

static jobject g_android_context;

struct _GAndroidCache
{
  struct
  {
    jclass klass;
    jmethodID get_files_dir;
    jmethodID get_external_files_dir;
    jmethodID get_cache_dir;
    jmethodID get_package_name;
    jmethodID get_application_info;
    jmethodID get_package_manager;
  } a_context;
  struct
  {
    jclass klass;
    jmethodID load_label;
  } a_package_item_info;
  struct
  {
    jclass klass;
    jmethodID get_absolute_path;
  } j_file;
  struct
  {
    jclass klass;
    jmethodID to_string;
  } j_object;
} g_android_cache;

/**
 * g_android_get_context:
 *
 * Get the reference to to the android.content.Context object set in
 * [func@GLib.android_set_context].
 *
 *
 * The
 * [Context](https://developer.android.com/reference/android/content/Context)
 * object is the central interface to interact with the Android
 * operating system.
 *
 * Do not call this function after [func@GLib.java_finalize] has been
 * called.
 *
 * Returns: the context object
 * Since: 2.86
 */
jobject
g_android_get_context (void)
{
  return g_android_context;
}

/**
 * g_android_set_context:
 * @context: the android.content.Context object
 *
 * Set the context object that glib shoud use when interacting with the
 * Android operating system.
 *
 *
 * GLib will only use the @context object of the initial call to this
 * function. All further calls are ignored.
 *
 * Returns: %TRUE if sucessful, %FALSE if this function has already been
 *   called
 * Since: 2.86
 */
gboolean
g_android_set_context (jobject context)
{
  g_return_val_if_fail (context != NULL, FALSE);
  if (g_android_context)
    return FALSE;

  GJavaScope env = g_java_enter_scope(4);
  g_android_context = (*env)->NewGlobalRef(env, context);

  jclass context_class = (*env)->FindClass(env, "android/content/Context");
  g_android_cache.a_context.klass = (*env)->NewGlobalRef(env, context_class);
  g_android_cache.a_context.get_files_dir = (*env)->GetMethodID(env,
                                                                g_android_cache.a_context.klass,
                                                                "getFilesDir",
                                                                "()Ljava/io/File;");
  g_android_cache.a_context.get_external_files_dir = (*env)->GetMethodID(env,
                                                                         g_android_cache.a_context.klass,
                                                                         "getExternalFilesDir",
                                                                         "(Ljava/lang/String;)Ljava/io/File;");
  g_android_cache.a_context.get_cache_dir = (*env)->GetMethodID(env,
                                                                g_android_cache.a_context.klass,
                                                                "getCacheDir",
                                                                "()Ljava/io/File;");
  g_android_cache.a_context.get_package_name = (*env)->GetMethodID(env,
                                                                   g_android_cache.a_context.klass,
                                                                   "getPackageName",
                                                                   "()Ljava/lang/String;");
  g_android_cache.a_context.get_application_info = (*env)->GetMethodID(env,
                                                                       g_android_cache.a_context.klass,
                                                                       "getApplicationInfo",
                                                                       "()Landroid/content/pm/ApplicationInfo;");
  g_android_cache.a_context.get_package_manager = (*env)->GetMethodID(env,
                                                                      g_android_cache.a_context.klass,
                                                                      "getPackageManager",
                                                                      "()Landroid/content/pm/PackageManager;");

  jclass package_item_info_class = (*env)->FindClass(env, "android/content/pm/PackageItemInfo");
  g_android_cache.a_package_item_info.klass = (*env)->NewGlobalRef(env, package_item_info_class);
  g_android_cache.a_package_item_info.load_label = (*env)->GetMethodID(env,
                                                                       g_android_cache.a_package_item_info.klass,
                                                                       "loadLabel",
                                                                       "(Landroid/content/pm/PackageManager;)Ljava/lang/CharSequence;");

  jclass file_class = (*env)->FindClass(env, "java/io/File");
  g_android_cache.j_file.klass = (*env)->NewGlobalRef(env, file_class);
  g_android_cache.j_file.get_absolute_path = (*env)->GetMethodID(env,
                                                                 g_android_cache.j_file.klass,
                                                                 "getAbsolutePath",
                                                                 "()Ljava/lang/String;");

  jclass object_class = (*env)->FindClass(env, "java/lang/Object");
  g_android_cache.j_object.klass = (*env)->NewGlobalRef(env, object_class);
  g_android_cache.j_object.to_string = (*env)->GetMethodID(env,
                                                           g_android_cache.j_object.klass,
                                                           "toString",
                                                           "()Ljava/lang/String;");

  g_java_leave_scope(&env);
  return TRUE;
}

void
g_android_finalize (void)
{
  JNIEnv *env = g_java_get_env();
  (*env)->DeleteGlobalRef(env, g_android_cache.a_context.klass);
  (*env)->DeleteGlobalRef(env, g_android_cache.a_package_item_info.klass);
  (*env)->DeleteGlobalRef(env, g_android_cache.j_file.klass);
  (*env)->DeleteGlobalRef(env, g_android_cache.j_object.klass);
  (*env)->DeleteGlobalRef(env, g_android_context);
  g_android_context = NULL;
}

static gchar *
g_android_call_dir_fun(jmethodID method, ...) {
  GJavaThreadSentinel *sentinel = g_java_enter_thread ();
  GJavaScope env = g_java_enter_scope(2);

  va_list args;
  va_start(args, method);
  jobject dir = (*env)->CallObjectMethodV(env,
                                          g_android_context,
                                          method,
                                          args);
  va_end(args);

  jobject path = (*env)->CallObjectMethod(env,
                                          dir,
                                          g_android_cache.j_file.get_absolute_path);
  gchar* ret = g_java_jstring_to_str(path, NULL);
  g_java_leave_scope(&env);
  g_java_leave_thread (sentinel);
  return ret;
}

gchar *
g_android_get_files_dir (void)
{
  return g_android_call_dir_fun(g_android_cache.a_context.get_files_dir);
}

gchar *
g_android_get_external_files_dir (void)
{
  return g_android_call_dir_fun(g_android_cache.a_context.get_files_dir,
                                NULL);
}

gchar *
g_android_get_cache_dir (void)
{
  return g_android_call_dir_fun(g_android_cache.a_context.get_cache_dir);
}


gchar *
g_android_get_package_name  (void)
{
  GJavaScope env = g_java_enter_scope(1);
  jobject name = (*env)->CallObjectMethod(env,
                                          g_android_context,
                                          g_android_cache.a_context.get_package_name);
  gchar* ret = g_java_jstring_to_str(name, NULL);
  g_java_leave_scope(&env);
  return ret;
}

gchar *
g_android_get_package_label (void)
{
  GJavaScope env = g_java_enter_scope(3);
  jobject pm = (*env)->CallObjectMethod(env,
                                        g_android_context,
                                        g_android_cache.a_context.get_package_manager);
  jobject info = (*env)->CallObjectMethod(env,
                                          g_android_context,
                                          g_android_cache.a_context.get_application_info);
  jobject label = (*env)->CallObjectMethod(env,
                                           info,
                                           g_android_cache.a_package_item_info.load_label,
                                           pm);
  gchar* ret = g_java_jstring_to_str(label, NULL);
  g_java_leave_scope(&env);
  return ret;
}


void
g_android_print_handler (const char *message)
{
  __android_log_print(ANDROID_LOG_INFO,
                      "print (stdout)",
                      "%s", message);
}

void
g_android_printerr_handler (const char *message)
{
  __android_log_print(ANDROID_LOG_WARN,
                      "print (stderr)",
                      "%s", message);
}

static int
g_android_log_level_to_android (GLogLevelFlags log_level)
{
  switch (log_level)
    {
    case G_LOG_LEVEL_ERROR:
      return ANDROID_LOG_FATAL;
    case G_LOG_LEVEL_CRITICAL:
      return ANDROID_LOG_ERROR;
    case G_LOG_LEVEL_WARNING:
      return ANDROID_LOG_WARN;
    case G_LOG_LEVEL_MESSAGE:
      return ANDROID_LOG_INFO;
    case G_LOG_LEVEL_INFO:
      return ANDROID_LOG_INFO;
    case G_LOG_LEVEL_DEBUG:
      return ANDROID_LOG_DEBUG;
    default:
      return ANDROID_LOG_WARN;
    }
}

void
g_android_log_handler (const gchar    *log_domain,
                       GLogLevelFlags  log_level,
                       const char     *message,
                       G_GNUC_UNUSED
                       gpointer        user_data)
{
  __android_log_print(g_android_log_level_to_android (log_level),
                      log_domain,
                      "%s", message);
}

GLogWriterOutput
g_android_structured_log_handler(GLogLevelFlags   log_level,
                                 const GLogField *fields,
                                 gsize            n_fields,
                                 G_GNUC_UNUSED
                                 gpointer         user_data) {
  const gchar* domain = NULL;
  const gchar* message = NULL;

  for (guint i = 0; (!domain || !message) && i < n_fields; i++)
  {
    const GLogField *field = &fields[i];
    if (g_strcmp0 (field->key, "GLIB_DOMAIN") == 0)
      domain = field->value;
    else if (g_strcmp0 (field->key, "MESSAGE") == 0)
      message = field->value;
  }

  if (!domain)
    domain = "**";

  if (!message)
    message = "(empty)";

  int rc = __android_log_print(g_android_log_level_to_android(log_level),
                               domain,
                               "%s", message);
  return rc == 0 ? G_LOG_WRITER_HANDLED : G_LOG_WRITER_UNHANDLED;
}
