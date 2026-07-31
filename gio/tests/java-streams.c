/* GLib testing framework examples and tests
 * Copyright (C) 2008 Red Hat, Inc
 *
 * SPDX-License-Identifier: LicenseRef-old-glib-tests
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <glib-java.h>

#include <gio/gio.h>
#include <gio/gjavafilestream.h>

#include <jni.h>

static struct {
  struct {
    jclass klass;
    jmethodID constructor;
    jmethodID create_temp_file;
    jmethodID get_path;
    jmethodID delete_on_exit;
  } j_file;
  struct {
    jclass klass;
    jmethodID constructor;
  } j_file_istream;
  struct {
    jclass klass;
    jmethodID constructor;
  } j_file_ostream;
  struct {
    jclass klass;
    jmethodID exit;
  } j_system;
} test_cache;

static void
java_streams_test_setup_cache (void)
{
  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 4);
  test_cache.j_file.klass = (*env)->NewGlobalRef (env, (*env)->FindClass (env, "java/io/File"));
  test_cache.j_file.constructor = (*env)->GetMethodID (env, test_cache.j_file.klass, "<init>", "(Ljava/lang/String;)V");
  test_cache.j_file.create_temp_file = (*env)->GetStaticMethodID (env, test_cache.j_file.klass, "createTempFile", "(Ljava/lang/String;Ljava/lang/String;)Ljava/io/File;");
  test_cache.j_file.get_path = (*env)->GetMethodID (env, test_cache.j_file.klass, "getPath", "()Ljava/lang/String;");
  test_cache.j_file.delete_on_exit = (*env)->GetMethodID (env, test_cache.j_file.klass, "deleteOnExit", "()V");
  test_cache.j_file_istream.klass = (*env)->NewGlobalRef (env, (*env)->FindClass (env, "java/io/FileInputStream"));
  test_cache.j_file_istream.constructor = (*env)->GetMethodID (env, test_cache.j_file_istream.klass, "<init>", "(Ljava/io/File;)V");
  test_cache.j_file_ostream.klass = (*env)->NewGlobalRef (env, (*env)->FindClass (env, "java/io/FileOutputStream"));
  test_cache.j_file_ostream.constructor = (*env)->GetMethodID (env, test_cache.j_file_ostream.klass, "<init>", "(Ljava/io/File;)V");
  test_cache.j_system.klass = (*env)->NewGlobalRef (env, (*env)->FindClass (env, "java/lang/System"));
  test_cache.j_system.exit = (*env)->GetStaticMethodID (env, test_cache.j_system.klass, "exit", "(I)V");
  (*env)->PopLocalFrame (env, NULL);

  g_assert_false (g_java_check_exception (NULL));
}

static void
java_tests_test_compare_input_streams (void)
{
  const gchar *srcdir = g_getenv ("G_TEST_SRCDIR");
  g_assert_nonnull (srcdir);
  gchar *path = g_build_filename(srcdir, "long-file-stream.txt", NULL);

  GJavaScope env = g_java_enter_scope (4);
  jstring jpath = g_java_str_to_jstring (path);
  jobject jfile = (*env)->NewObject (env, test_cache.j_file.klass,
                                    test_cache.j_file.constructor,
                                    jpath);
  g_assert_false (g_java_check_exception (NULL));
  jobject jstream = (*env)->NewObject (env, test_cache.j_file_istream.klass,
                                       test_cache.j_file_istream.constructor,
                                       jfile);
  g_assert_false (g_java_check_exception (NULL));
  GFileInputStream *astream = g_java_file_input_stream_new_for_java (env, jstream);
  g_java_leave_scope (&env);

  GFile *file = g_file_new_for_path (path);
  GError* err = NULL;
  GFileInputStream *bstream = g_file_read (file, NULL, &err);
  g_assert_no_error (err);

  guchar abuffer[512], bbuffer[512];
  gssize len;
  do {
    len = g_input_stream_read (G_INPUT_STREAM (astream), abuffer, sizeof abuffer, NULL, &err);
    g_assert_no_error (err);
    g_assert_cmpint (len, ==, g_input_stream_read (G_INPUT_STREAM (bstream), bbuffer, sizeof abuffer, NULL, &err));
    g_assert_no_error (err);
    g_assert_true (memcmp (abuffer, bbuffer, (gsize)len) == 0);
  } while (len > 0);

  g_object_unref (astream);
  g_object_unref (bstream);

  g_free (path);
}

static void
java_tests_test_file_write_output_stream (void)
{
  const gchar *test_contents = "Nullus liber homo capiatur, vel imprisonetur, aut disseisiatur, aut utlagetur, aut exuletur, aut aliquo modo destruatur, nec super eum ibimus, nec super eum mittemus, nisi per legale judicium parium suorum vel per legem terre.\n";

  GJavaScope env = g_java_enter_scope (4);
  jobject prefix = g_java_str_to_jstring ("GIO-java-streams.");
  jobject jfile = (*env)->CallStaticObjectMethod (env, test_cache.j_file.klass,
                                                  test_cache.j_file.create_temp_file,
                                                  prefix, NULL);
  (*env)->CallVoidMethod (env, jfile,
                          test_cache.j_file.delete_on_exit);
  g_assert_false (g_java_check_exception (NULL));
  jobject jstream = (*env)->NewObject (env, test_cache.j_file_ostream.klass,
                                       test_cache.j_file_ostream.constructor,
                                       jfile);
  g_assert_false (g_java_check_exception (NULL));
  GFileOutputStream *astream = g_java_file_output_stream_new_for_java (env, jstream);

  jobject jpath = (*env)->CallObjectMethod (env, jfile,
                                            test_cache.j_file.get_path);
  gchar *path = g_java_jstring_to_str (jpath, NULL);
  g_java_leave_scope (&env);

  GError *err = NULL;
  g_output_stream_write (G_OUTPUT_STREAM (astream), test_contents, strlen (test_contents), NULL, &err);
  g_assert_no_error (err);

  g_output_stream_flush (G_OUTPUT_STREAM (astream), NULL, &err);
  g_assert_no_error (err);

  GFile *file = g_file_new_for_path (path);
  GFileInputStream *bstream = g_file_read (file, NULL, &err);
  g_assert_no_error (err);

  gchar buffer[512];
  for (gsize i = 0; i < strlen (test_contents); )
    {
      gssize len = g_input_stream_read (G_INPUT_STREAM (bstream), buffer, sizeof buffer, NULL, &err);
      g_assert_cmpint (len, >, 0);
      g_assert_true (strncmp (&test_contents[i], buffer, len) == 0);
      i += len;
    }

  g_object_unref (astream);
  g_object_unref (bstream);

  g_free (path);
}

int
main (int   argc,
      char *argv[])
{
  JavaVM *vm;
  JNIEnv *env;

  g_test_init (&argc, &argv, NULL);

  gint rc = JNI_CreateJavaVM(&vm, (void**)&env,
                             &(JavaVMInitArgs){
                               .version = JNI_VERSION_1_6,
                               .nOptions = 0,
                               .options = NULL,
                               .ignoreUnrecognized = FALSE
                             });

  g_assert_cmpint (rc, ==, JNI_OK);

  glib_java_initialize (vm, NULL);

  java_streams_test_setup_cache ();

  g_test_add_func ("/java-streams/compare-input", java_tests_test_compare_input_streams);
  g_test_add_func ("/java-streams/write-output", java_tests_test_file_write_output_stream);

  gint ret = g_test_run();

  /* Use System.exit(ret) that the JVM can cleanup created temp files */
  (*env)->CallStaticVoidMethod (env, test_cache.j_system.klass,
                                test_cache.j_system.exit,
                                ret);
  g_return_val_if_reached (ret);
}
