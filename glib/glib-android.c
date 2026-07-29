/* GLIB - Library of useful routines for C programming
 * Copyright (c) 2024-26  Florian "sp1rit" <sp1rit@disroot.org>
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
#include "gtestutils.h"

#include <pthread.h>

#include "glib-android.h"

static struct {
  gboolean  initialized;
  /*
   * RecMutex in case something in glib_android_initialize ends up
   * calling glib_android_is_initialized (such as the g_log handler)
   */
  GRecMutex lock;
} g_android_init;

static JavaVM *g_android_jvm = NULL;
static jobject g_android_class_loader = NULL;
static jobject g_android_context = NULL;

static struct
{
  struct
  {
    jclass klass;
    jmethodID load_class;
  } j_classloader;
  struct
  {
    jclass klass;
    jmethodID get_application_context;
  } a_context;
} g_android_cache;

static __thread JNIEnv *g_android_jvm_thread = NULL;
static pthread_key_t g_android_jvm_thread_cleanup_key;

static void
g_android_jvm_thread_cleanup (G_GNUC_UNUSED void *env)
{
  int32_t rc = (*g_android_jvm)->DetachCurrentThread (g_android_jvm);
  if (G_UNLIKELY (rc != JNI_OK))
    g_critical ("Java: Unable to detach thread from JVM (%d)", rc);
}


/**
 * glib_android_initialize:
 * @vm: the active JVM
 * @class_loader: (nullable): the classloader to use
 * @context: (nullable): The Android Context object used to interface with the OS
 *
 * Initialize the Android application handling routines of GLib.
 *
 *
 * This function should be called before interacting with any other
 * Android related functionality in GLib. It stores the global JVM
 * reference that the other functionality will use in order to interact
 * with the JVM.
 *
 * You may also pass an alternative user class loader, which
 * [func@GLib.android_find_class] will use instead of the *current*
 * class loader if available. Note that GLib will not use the
 * overridden class loader to resolve classes from java.base internally
 * and expects the *current* class loader to be capable of doing so.
 *
 * Returns: %TRUE if successful, %FALSE otherwise
 * Since: 2.88
 */
gboolean
glib_android_initialize (JavaVM *vm,
                         jobject class_loader,
                         jobject context)
{
  // fast-path after initialization
  if (g_atomic_int_get (&g_android_init.initialized))
    return FALSE;


  g_rec_mutex_lock (&g_android_init.lock);
  if (g_android_jvm)
    {
      g_rec_mutex_unlock (&g_android_init.lock);
      return FALSE;
    }

  g_android_jvm = vm;
  pthread_key_create (&g_android_jvm_thread_cleanup_key, g_android_jvm_thread_cleanup);

  GAndroidJvmScope env = g_android_enter_jvm_scope (3);

  jclass classloader_class = (*env)->FindClass (env, "java/lang/ClassLoader");
  g_android_cache.j_classloader.klass = (*env)->NewGlobalRef (env, classloader_class);
  g_android_cache.j_classloader.load_class = (*env)->GetMethodID (env,
                                                                  g_android_cache.j_classloader.klass,
                                                                  "loadClass",
                                                                  "(Ljava/lang/String;)Ljava/lang/Class;");

  jclass context_class = (*env)->FindClass (env, "android/content/Context");
  g_android_cache.a_context.klass = (*env)->NewGlobalRef (env, context_class);
  g_android_cache.a_context.get_application_context = (*env)->GetMethodID (env,
                                                                           g_android_cache.a_context.klass,
                                                                           "getApplicationContext",
                                                                           "()Landroid/content/Context;");

  g_android_class_loader = NULL;
  if (class_loader)
    {
      if ((*env)->IsInstanceOf (env, class_loader, classloader_class))
        g_android_class_loader = (*env)->NewGlobalRef (env, class_loader);
      else
        g_critical ("ClassLoader passed to glib_android_initialize is not a valid class loader");
    }

  g_android_context = NULL;
  if (context)
    {
      if ((*env)->IsInstanceOf (env, context, context_class))
        {
          jobject *appl_context = (*env)->CallObjectMethod (env,
                                                            context,
                                                            g_android_cache.a_context.get_application_context);
          g_android_context = (*env)->NewGlobalRef (env, appl_context);
        }
      else
        g_critical ("Context passed to glib_android_initialize is not a valid context");
    }

  g_android_leave_jvm_scope (&env);
  g_atomic_int_set (&g_android_init.initialized, TRUE);
  g_rec_mutex_unlock (&g_android_init.lock);
  return TRUE;
}


/**
 * glib_android_is_initialized:
 *
 * Test if GLib has its android application handling facilities
 * initialized.
 *
 *
 * This will be the case once [func@GLib.android_initialize] was called
 * with a valid reference.
 *
 * Returns: %TRUE if the facilities are initialized
 * Since: 2.88
 */
gboolean
glib_android_is_initialized (void)
{
  // fast-path after initialization
  if (g_atomic_int_get (&g_android_init.initialized))
    return TRUE;

  g_rec_mutex_lock (&g_android_init.lock);
  gboolean ret = g_android_jvm != NULL;
  g_rec_mutex_unlock (&g_android_init.lock);
  return ret;
}


/**
 * g_android_get_env:
 *
 * Get the environment associated with the current thread.
 *
 * You have to ensure that the current thread is actually attached to
 * the JVM before calling this function. If are uncertain about the
 * state of the current thread, use [func@GLib.android_jvm_enter_thread]
 * instead.
 *
 * Returns: the java environment
 * Since: 2.88
 */
JNIEnv *
g_android_get_env (void)
{
  int32_t rc;
  JavaVMAttachArgs args;

  if (G_LIKELY(g_android_jvm_thread))
    return g_android_jvm_thread;
  g_return_val_if_fail (g_android_jvm != NULL, NULL);
  rc = (*g_android_jvm)->GetEnv (g_android_jvm, (void **)&g_android_jvm_thread, JNI_VERSION_1_6);
  if (rc == JNI_OK)
    return g_android_jvm_thread;

  args.version = JNI_VERSION_1_6;
  args.name = "GDK Thread Environment";
  args.group = NULL;
  rc = (*g_android_jvm)->AttachCurrentThread (g_android_jvm, &g_android_jvm_thread, &args);
  if (G_UNLIKELY (rc != JNI_OK))
    {
      g_critical ("Android: Unable to attach current thread to the JVM (%d)", rc);
      g_android_jvm_thread = NULL;
      return NULL;
    }

  pthread_setspecific (g_android_jvm_thread_cleanup_key, g_android_jvm_thread);
  return g_android_jvm_thread;
}

/**
 * g_android_get_class_loader:
 *
 * Get the alternative user classloader passed to
 * [func@GLib.android_initialize].
 *
 * Returns: (nullable): the classloader or %NULL if not set
 * Since: 2.88
 */
jobject
g_android_get_class_loader (void)
{
  return g_android_class_loader;
}

/**
 * g_android_find_class:
 * @klass: Class name to look for (JVM notation)
 *
 * Search for a class using the alternative user class loader set in
 * [func@GLib.android_initialize] or the *current* class loader if the
 * user class loader wasn't set.
 *
 * Returns: (nullable): local ref'd class or %NULL if no class was found
 * Since: 2.88
 */
jclass
g_android_find_class (const gchar *klass)
{
  GAndroidJvmScope env = g_android_enter_jvm_scope (2);
  jclass java_class;
  if (g_android_class_loader)
    {
      jstring class_name = (*env)->NewStringUTF (env, klass); // Note: this breaks if klass contains
                                                              //       high codepoints. Not sure if
                                                              //       class names are even allowed
                                                              //       to contain them, but still...
      java_class = (*env)->CallObjectMethod (env,
                                             g_android_class_loader,
                                             g_android_cache.j_classloader.load_class,
                                             class_name);
    }
  else
    {
      java_class = (*env)->FindClass (env, klass);
    }

  return g_android_leave_jvm_scope_with_ref (&env, java_class);
}

/**
 * g_android_get_context:
 *
 * Get the reference to to the android.content.Context object set in
 * [func@GLib.android_initialize].
 *
 *
 * The
 * [Context](https://developer.android.com/reference/android/content/Context)
 * object is the central interface to interact with the Android
 * operating system.
 *
 * Returns: (nullable): the context object
 * Since: 2.88
 */
jobject
g_android_get_context (void)
{
  return g_android_context;
}

/**
 * g_android_enter_jvm_scope:
 * @capacity: number of local references used in the scope
 *
 * Enter a new JNI local reference frame.
 *
 *
 * The frame keeps track of newly created local references, which are
 * automatically released once [func@GLib.android_leave_jvm_scope] is
 * called.
 *
 * This method ensures that the new frame has at least enough capacity
 * to keep track of @capacity local references.
 *
 * Returns: the JNI environment for the current thread
 * Since: 2.88
 */
GAndroidJvmScope
g_android_enter_jvm_scope (int32_t capacity)
{
  JNIEnv *env = g_android_get_env ();
  int32_t rc = (*env)->PushLocalFrame (env, capacity);
  if (G_UNLIKELY (rc != JNI_OK))
    g_error("Java: Failed to enter local frame (Error %d)", rc);
  return env;
}

/**
 * g_android_leave_jvm_scope:
 * @self: the scope to exit
 *
 * Exit the current JNI local reference frame created by
 * [func@GLib.android_enter_jvm_scope].
 *
 *
 * This releases all local references held by the current local
 * reference frame.
 *
 * Since: 2.88
 */
void
g_android_leave_jvm_scope (GAndroidJvmScope *self)
{
  g_return_if_fail (self != NULL && *self != NULL);
  (**self)->PopLocalFrame (*self, NULL);
}

/**
 * g_android_leave_jvm_scope_with_ref:
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
 * Since: 2.88
 */
jobject
g_android_leave_jvm_scope_with_ref (GAndroidJvmScope *self, jobject ref)
{
  g_return_val_if_fail (self != NULL && *self != NULL, NULL);
  return (**self)->PopLocalFrame (*self, ref);
}
