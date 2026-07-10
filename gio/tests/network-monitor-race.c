/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * licence, or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/glib.h>
#include <gio/gio.h>

#define MAX_RUNS 20

static gboolean
quit_loop (gpointer user_data)
{
  g_main_loop_quit (user_data);

  return FALSE;
}

static gpointer
thread_func (gpointer user_data)
{
  g_network_monitor_get_default ();
  g_timeout_add (100, quit_loop, user_data);

  return NULL;
}

static gboolean
call_func (gpointer user_data)
{
  GThread *thread;

  thread = g_thread_new (NULL, thread_func, user_data);
  g_thread_unref (thread);

  return FALSE;
}

/* Test that calling g_network_monitor_get_default() in a thread doesn’t cause
 * a crash. This is a probabilistic test; since it’s testing a race condition,
 * it can’t deterministically reproduce the problem. The threading has to
 * happen in subprocesses, since the result of g_network_monitor_get_default()
 * is unavoidably cached once created. */
static void
test_network_monitor (void)
{
  guint ii;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=793727");

  if (g_test_subprocess ())
    {
       GMainLoop *main_loop;

       main_loop = g_main_loop_new (NULL, FALSE);
       g_timeout_add (1, call_func, main_loop);
       g_main_loop_run (main_loop);
       g_main_loop_unref (main_loop);

       return;
    }

  for (ii = 0; ii < MAX_RUNS; ii++)
    {
       g_test_trap_subprocess (NULL,
                               0,
                               G_TEST_SUBPROCESS_INHERIT_STDOUT |
                               G_TEST_SUBPROCESS_INHERIT_STDERR);
       g_test_trap_assert_passed ();
    }
}

static void
async_result_cb (GObject      *source_object,
                 GAsyncResult *result,
                 void         *user_data)
{
  GAsyncResult **result_out = user_data;

  g_assert (result_out != NULL);
  g_assert (*result_out == NULL);

  *result_out = g_object_ref (result);

  g_main_context_wakeup (g_main_context_get_thread_default ());
}

static void *
async_thread_cb (void *user_data)
{
  int *go = user_data;  /* (atomic) */
  GMainContext *context = NULL;
  GAsyncResult *result = NULL;
  GNetworkMonitor *monitor = NULL;
  GError *local_error = NULL;

  context = g_main_context_new ();
  g_main_context_push_thread_default (context);

  /* Spin until all the threads are ready. */
  while (!g_atomic_int_get (go));

  g_network_monitor_get_default_async (NULL, async_result_cb, &result);

  while (result == NULL)
    g_main_context_iteration (context, TRUE);

  monitor = g_network_monitor_get_default_finish (result, &local_error);
  g_assert_no_error (local_error);
  g_assert_nonnull (result);

  g_main_context_pop_thread_default (context);
  g_clear_pointer (&context, g_main_context_unref);
  g_clear_object (&result);

  return g_steal_pointer (&monitor);
}

static void
test_network_monitor_async (void)
{
  g_test_summary ("Test that calling g_network_monitor_get_default() async in parallel returns the same result");

  if (!g_test_subprocess ())
    {
      for (unsigned int ii = 0; ii < MAX_RUNS; ii++)
        {
           g_test_trap_subprocess (NULL,
                                   0,
                                   G_TEST_SUBPROCESS_INHERIT_STDOUT |
                                   G_TEST_SUBPROCESS_INHERIT_STDERR);
           g_test_trap_assert_passed ();
        }
    }
  else
    {
      GThread *threads[20] = { NULL, };
      GNetworkMonitor *results[20] = { NULL, };
      int go = 0;  /* (atomic) */

      for (size_t i = 0; i < G_N_ELEMENTS (threads); i++)
        threads[i] = g_thread_new (NULL, async_thread_cb, &go);

      g_atomic_int_set (&go, 1);

      for (size_t i = 0; i < G_N_ELEMENTS (threads); i++)
        results[i] = g_thread_join (g_steal_pointer (&threads[i]));

      for (size_t i = 0; i < G_N_ELEMENTS (threads) - 1; i++)
        {
          g_assert_true (G_IS_NETWORK_MONITOR (results[i]));
          g_assert_true (results[i] == results[i + 1]);
        }

      for (size_t i = 0; i < G_N_ELEMENTS (threads); i++)
        g_clear_object (&results[i]);
    }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/network-monitor/create-in-thread",
                   test_network_monitor);
  g_test_add_func ("/network-monitor/create-in-thread/async",
                   test_network_monitor_async);

  return g_test_run ();
}
