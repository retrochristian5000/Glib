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

#include "gmessages.h"
#include "grefcount.h"
#include "gtestutils.h"

#ifdef G_PLATFORM_ANDROID
#include "glib-androidprivate.h"
#endif

#include "glib-java.h"

static JavaVM *g_java_vm;
static jobject g_java_class_loader;

struct _GJavaCache
{
  struct
  {
    jclass klass;
    jmethodID load_class;
  } classloader;
  struct
  {
    jclass klass;
  } char_conv_exception;
  struct
  {
    jclass klass;
    jmethodID get_message;
  } throwable;
} g_java_cache;

struct _GJavaThreadSentinel
{
  JNIEnv *env;
  gboolean needs_detach;
  grefcount rc;
};
static __thread GJavaThreadSentinel
g_java_thread = { .env = NULL, .needs_detach = FALSE };

/**
 * glib_java_initialize:
 * @vm: the active JVM
 * @class_loader: (nullable): the classloader to use
 *
 * Initialize the Java handling routines of GLib.
 *
 *
 * This function should be called before interacting with any other
 * Java-related functionality in GLib. It stores the global JVM
 * reference that the other functionality will use in order to interact
 * with the JVM.
 *
 * You may also pass an alternative user class loader, which
 * [func@GLib.java_find_class] will use instead of the *current* class
 * loader if available. Note that GLib will not use the overridden class
 * loader to resolve classes from java.base internally and expects the
 * *current* class loader to be capable of doing so.
 *
 * Returns: %TRUE if successful, %FALSE otherwise
 * Since: 2.86
 */
gboolean
glib_java_initialize (JavaVM *vm,
                      jobject class_loader)
{
  if (g_java_vm)
    return FALSE;
  g_java_vm = vm;

  GJavaScope env = g_java_enter_scope(3);
  g_java_class_loader = class_loader ?
      (*env)->NewGlobalRef(env, class_loader) : NULL;

  jclass classloader_class = (*env)->FindClass(env, "java/lang/ClassLoader");
  g_java_cache.classloader.klass = (*env)->NewGlobalRef(env, classloader_class);
  g_java_cache.classloader.load_class = (*env)->GetMethodID(env,
                                                            g_java_cache.classloader.klass,
                                                            "loadClass",
                                                            "(Ljava/lang/String;)Ljava/lang/Class;");

  jclass char_conv_exception = (*env)->FindClass(env, "java/io/CharConversionException");
  g_java_cache.char_conv_exception.klass = (*env)->NewGlobalRef(env, char_conv_exception);

  jclass throwable = (*env)->FindClass (env, "java/lang/Throwable");
  g_java_cache.throwable.klass = (*env)->NewGlobalRef (env, throwable);
  g_java_cache.throwable.get_message = (*env)->GetMethodID (env, g_java_cache.throwable.klass, "getMessage", "()Ljava/lang/String;");

  g_java_leave_scope(&env);
  return TRUE;
}


/**
 * glib_java_is_initialized:
 *
 * Test if GLib is currently aware of a JVM reference.
 *
 *
 * This will be the case once [func@GLib.java_initialize] was called
 * with a valid reference, until the point [func@GLib.java_finalize] was
 * called.
 *
 * Returns: %TRUE if the global JVM reference is set
 * Since: 2.86
 */
gboolean
glib_java_is_initialized (void)
{
  return g_java_vm != NULL;
}

/**
 * glib_java_finalize:
 *
 * Free everything related to the java handling routines.
 *
 *
 * Should be called shortly before the application exits. In theory not
 * calling this function before exiting, a badly behaved JVM
 * implementation might act in unexpected ways.
 *
 * Since: 2.86
 */
void
glib_java_finalize (void)
{
  g_return_if_fail (g_java_vm != NULL);
#ifdef G_PLATFORM_ANDROID
  g_android_finalize();
#endif

  JNIEnv *env = g_java_get_env();
  g_return_if_fail (g_java_thread.needs_detach == FALSE);
  gboolean finished = g_ref_count_dec (&g_java_thread.rc);
  g_return_if_fail (finished);

  if (g_java_class_loader) {
    (*env)->DeleteGlobalRef(env, g_java_class_loader);
    g_java_class_loader = NULL;
  }

  (*env)->DeleteGlobalRef(env, g_java_cache.classloader.klass);
  (*env)->DeleteGlobalRef(env, g_java_cache.char_conv_exception.klass);
  (*env)->DeleteGlobalRef(env, g_java_cache.throwable.klass);

  g_java_thread.env = NULL;
  g_java_vm = NULL;
}

/**
 * g_java_get_env:
 *
 * Get the environment associated with the current thread.
 *
 * You have to ensure that the current thread is actually attached to
 * the JVM before calling this function. If are uncertain about the
 * state of the current thread, use [func@GLib.java_enter_thread]
 * instead.
 *
 * Returns: the java environment
 * Since: 2.86
 */
JNIEnv *
g_java_get_env (void)
{
  if (g_java_thread.env)
    return g_java_thread.env;
  g_return_val_if_fail (g_java_vm != NULL, NULL);
  gint rc = (*g_java_vm)->GetEnv(g_java_vm, (void**)&g_java_thread.env, JNI_VERSION_1_6);
  if (G_UNLIKELY(rc != JNI_OK))
    {
      g_critical("Java: Failure getting environment for current thread (%d). Is is attached?", rc);
      return NULL;
    }
  g_java_thread.needs_detach = FALSE;
  g_ref_count_init (&g_java_thread.rc);
  return g_java_thread.env;
}

/**
 * g_java_enter_thread:
 *
 * Attach the current thread to the JVM if it isn't already.
 *
 *
 * If it is, increase the ref-count of the thread sentinel by one.
 *
 * Returns: (transfer full): the thread sentinel
 * Since: 2.86
 */
GJavaThreadSentinel *
g_java_enter_thread (void)
{
  g_return_val_if_fail (g_java_vm != NULL, NULL);

  if (g_java_thread.env)
    {
      g_ref_count_inc (&g_java_thread.rc);
      return &g_java_thread;
    }

  gint rc = (*g_java_vm)->GetEnv (g_java_vm, (void **) &g_java_thread.env, JNI_VERSION_1_6);
  if (rc == JNI_OK)
    {
      g_java_thread.needs_detach = FALSE;
      g_ref_count_init (&g_java_thread.rc);
    }
  else if (rc == JNI_EDETACHED)
    {
      JavaVMAttachArgs args = {
        .version = JNI_VERSION_1_6,
        .name = "GDK Thread Environment",
        .group = NULL
      };
#ifdef G_PLATFORM_ANDROID
      rc = (*g_java_vm)->AttachCurrentThread (g_java_vm, &g_java_thread.env, &args);
#else // OpenJDK JVM
      rc = (*g_java_vm)->AttachCurrentThread (g_java_vm, (void**)&g_java_thread.env, &args);
#endif
      if (G_UNLIKELY (rc != JNI_OK))
        {
          g_critical ("Java: Unable to attach current thread to the JVM (%d)", rc);
          g_java_thread.env = NULL;
          g_java_thread.needs_detach = FALSE;
        }
      else
        {
          g_java_thread.needs_detach = TRUE;
          g_ref_count_init (&g_java_thread.rc);
        }
    }
  else
    {
      g_critical ("Java: Unexpected failure getting environment for current thread (%d)", rc);
      g_java_thread.env = NULL;
      g_java_thread.needs_detach = FALSE;
    }
  return &g_java_thread;
}

/**
 * g_java_leave_thread:
 * @self: the thread sentinel
 *
 * Detach the current thread to the JVM if @self is the last reference.
 *
 * Since: 2.86
 */
void
g_java_leave_thread (GJavaThreadSentinel *self)
{
  g_return_if_fail (g_java_vm != NULL);
  g_assert (self == &g_java_thread);

  if (g_ref_count_dec (&self->rc) && self->needs_detach)
    {
      gint rc = (*g_java_vm)->DetachCurrentThread (g_java_vm);
      if (G_UNLIKELY (rc != JNI_OK))
        g_critical ("Java: Unable to detach thread from JVM (%d)", rc);
      g_java_thread.env = NULL;
      g_java_thread.needs_detach = FALSE;
    }
}

/**
 * g_java_get_class_loader:
 *
 * Get the alternative user classloader passed to
 * [func@GLib.java_initialize].
 *
 * Returns: (nullable): the classloader or %NULL if not set
 * Since: 2.86
 */
jobject
g_java_get_class_loader (void)
{
  return g_java_class_loader;
}

/**
 * g_java_enter_scope:
 * @capacity: number of local references used in the scope
 *
 * Enter a new JNI local reference frame.
 *
 *
 * The frame keeps track of newly created local references, which are
 * automatically released once [func@GLib.java_leave_scope] is called.
 *
 * This method ensures that the new frame has at least enough capacity
 * to keep track of @capacity local references.
 *
 * Returns: the java environment for the current thread
 * Since: 2.86
 */
GJavaScope
g_java_enter_scope (gint capacity)
{
  JNIEnv *env = g_java_get_env();
  gint rc = (*env)->PushLocalFrame(env, capacity);
  if (G_UNLIKELY(rc != JNI_OK))
    g_error("Java: Failed to enter local frame (Error %d)", rc);
  return env;
}

/**
 * g_java_leave_scope:
 * @self: the scope to exit
 *
 * Exit the current JNI local reference frame created by
 * [func@GLib.java_enter_scope].
 *
 *
 * This releases all local references held by the current local
 * reference frame.
 *
 * Since: 2.86
 */
void
g_java_leave_scope (GJavaScope* self)
{
  g_return_if_fail(self != NULL && *self != NULL);
  (**self)->PopLocalFrame(*self, NULL);
}

/**
 * g_java_leave_scope_with_ref:
 * @self: the scope to exit
 * @ref: the object to retain a local reference of
 *
 * Exit the current JNI locale reference frame while moving a local
 * reference (@ref) to the previous local reference frame.
 *
 *
 * This is mainly useful when writing a function, which returns a java
 * object.
 *
 * Returns: @ref as a reference from the scope above.
 * Since: 2.86
 */
jobject
g_java_leave_scope_with_ref (GJavaScope* self, jobject ref)
{
  g_return_val_if_fail(self != NULL && *self != NULL, NULL);
  return (**self)->PopLocalFrame(*self, ref);
}

/**
 * g_java_find_class:
 * @klass: Class name to look for (JVM notation)
 *
 * Search for a class using the alternative user class loader set in
 * [func@GLib.java_initialize] or the *current* class loader if the
 * user class loader wasn't set.
 *
 * Returns: (nullable): local ref'd class or %NULL if no class was found
 * Since: 2.86
 */

jclass
g_java_find_class (const gchar* klass)
{
  GJavaScope env = g_java_enter_scope(2);
  jclass java_class;
  if (g_java_class_loader)
    {
      jstring class_name = g_java_str_to_jstring(klass);
      java_class = (*env)->CallObjectMethod(env,
                                            g_java_class_loader,
                                            g_java_cache.classloader.load_class,
                                            class_name);
    }
  else
    {
      java_class = (*env)->FindClass(env, klass);
    }

  return g_java_leave_scope_with_ref(&env, java_class);
}

/**
 * g_java_strn_to_jstring:
 * @str: (array length=len): string to convert
 * @len: the length of @str, -1 if @str is NULL terminated
 *
 * Converts an UTF-8 string into a java.lang.String object.
 *
 * Returns: local ref'd string object
 * Since: 2.86
 */
jstring
g_java_strn_to_jstring (const gchar *str, gssize len)
{
  if (!str)
    return NULL;

  GJavaScope env = g_java_enter_scope(1);

  glong conv_len;
  GError *err = NULL;
  gunichar2* utf16 = g_utf8_to_utf16 (str, len,
                                      NULL, &conv_len,
                                      &err);
  jstring ret;
  if (err)
    {
      (*env)->ThrowNew(env,
                       g_java_cache.char_conv_exception.klass,
                       err->message);
      g_error_free(err);
      ret = NULL;
    }
  else
    {
      ret = (*env)->NewString(env, utf16, conv_len);
      g_free(utf16);
    }

  return g_java_leave_scope_with_ref(&env, ret);
}

/**
 * g_java_str_to_jstring:
 * @str: string to convert
 *
 * Converts an UTF-8 string into a java.lang.String object.
 *
 * Returns: local ref'd string object
 * Since: 2.86
 */
jstring
g_java_str_to_jstring (const gchar *str)
{
  return g_java_strn_to_jstring(str, -1);
}

/**
 * g_java_jstring_to_str:
 * @string: the java.lang.String object
 * @len: (out) (optional): the length of the returned string
 *
 * Converts a Java string into a null terminated UTF-8 string.
 *
 *
 * Note: while the returned string is null terminated, there might be
 * data past the initial NULL byte, if @string contained NUL characters.
 * Consider using @len in such cases.
 *
 * Returns: (transfer full): converted UTF-8 string
 * Since: 2.86
 */
gchar*
g_java_jstring_to_str (jstring string, gsize *len)
{
  if (!string)
    {
      if (len)
        *len = 0;
      return NULL;
    }

  JNIEnv* env = g_java_get_env();

  jsize jlen = (*env)->GetStringLength(env, string);
  const jchar* utf16 = (*env)->GetStringChars(env, string, NULL);

  GError *err = NULL;
  gchar* utf8 = g_utf16_to_utf8(utf16, jlen, NULL, (glong*)len, &err);
  if (err)
    {
      (*env)->ThrowNew(env,
                       g_java_cache.char_conv_exception.klass,
                       err->message);
      g_error_free(err);
    }
  (*env)->ReleaseStringChars(env, string, utf16);
  return utf8;
}

G_DEFINE_QUARK (G_JAVA_ERROR, g_java_error)

/**
 * g_java_check_exception:
 * @error: return location for a [struct@GLib.Error], or %NULL
 *
 * Determines if the JVM has a pending exception and if it does, clears
 * it and rethrows it as @error ([error@GLib.JavaError.EXCEPTION]).
 *
 * Returns: %TRUE if there was an exception, %FALSE otherwise
 * Since: 2.86
 */

gboolean
g_java_check_exception (GError **error)
{
  GJavaScope env = g_java_enter_scope(2);

  jthrowable exception = (*env)->ExceptionOccurred (env);
  if (exception)
    {
      (*env)->ExceptionClear (env);
      if (error)
        {
          jstring msg = (*env)->CallObjectMethod (env, exception,
                                                  g_java_cache.throwable.get_message);
          gchar *cmsg = g_java_jstring_to_str (msg, NULL);
          g_set_error_literal (error, G_JAVA_ERROR, G_JAVA_EXCEPTION, cmsg);
          g_free (cmsg);
        }

      g_java_leave_scope (&env);
      return TRUE;
    }

  g_java_leave_scope (&env);
  return FALSE;
}
