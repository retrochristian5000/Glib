/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2026 Sorah Fukumori
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include "gasyncinitable.h"
#include "gnetworkmonitorsystemd.h"
#include "gcancellable.h"
#include "gioerror.h"
#include "ginitable.h"
#include "giomodule-priv.h"
#include "gnetworkmonitor.h"
#include "gdbusproxy.h"
#include "gtask.h"

#define G_NETWORK_MONITOR_SYSTEMD_GET_INITABLE_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), G_TYPE_INITABLE, GInitable))

static void g_network_monitor_systemd_iface_init (GNetworkMonitorInterface *iface);
static void g_network_monitor_systemd_initable_iface_init (GInitableIface *iface);
static void g_network_monitor_systemd_async_initable_iface_init (GAsyncInitableIface *iface);

typedef enum
{
  PROP_NETWORK_AVAILABLE = 1,
  PROP_NETWORK_METERED,
  PROP_CONNECTIVITY
} GNetworkMonitorSystemdProperty;

/* Network monitor backend for systemd-networkd, via its
 * org.freedesktop.network1.Manager D-Bus interface. Ranks below the
 * NetworkManager backend, which reports richer system-wide state.
 *
 * This backend initializes itself asynchronously, to avoid blocking D-Bus
 * round trips during initable_init(). Until the connection to
 * systemd-networkd is established, network-available is FALSE. If networkd
 * is unavailable or not authoritative, it falls back to the netlink backend.
 * can_reach() is inherited from the netlink parent and works in every
 * state. */
struct _GNetworkMonitorSystemd
{
  GNetworkMonitorNetlink parent_instance;

  GCancellable *cancellable;  /* (owned) */

  /* proxy is NULL while the async proxy setup is still pending. */
  GDBusProxy *proxy;  /* (owned) (nullable) */
  unsigned long signal_id;

  GNetworkConnectivity connectivity;
  gboolean network_available;
};

G_DEFINE_TYPE_WITH_CODE (GNetworkMonitorSystemd, g_network_monitor_systemd, G_TYPE_NETWORK_MONITOR_NETLINK,
                         G_IMPLEMENT_INTERFACE (G_TYPE_NETWORK_MONITOR,
                                                g_network_monitor_systemd_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                g_network_monitor_systemd_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE,
                                                g_network_monitor_systemd_async_initable_iface_init)
                         _g_io_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_NETWORK_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "systemd",
                                                         25))

static void
g_network_monitor_systemd_init (GNetworkMonitorSystemd *self)
{
  self->connectivity = G_NETWORK_CONNECTIVITY_LOCAL;
  self->network_available = FALSE;
}

static void
g_network_monitor_systemd_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GNetworkMonitorSystemd *self = G_NETWORK_MONITOR_SYSTEMD (object);

  switch ((GNetworkMonitorSystemdProperty) prop_id)
    {
    case PROP_NETWORK_AVAILABLE:
      g_value_set_boolean (value, self->network_available);
      break;

    case PROP_NETWORK_METERED:
      /* systemd-networkd does not expose metered state. */
      g_value_set_boolean (value, FALSE);
      break;

    case PROP_CONNECTIVITY:
      g_value_set_enum (value, self->connectivity);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* "routable" means at least one managed link has a global address, i.e. the host
 * can reach off-link destinations. OperationalState is used rather than
 * OnlineState because OnlineState is shaped by RequiredForOnline= policy and
 * treats link-local-only links as online. */
static gboolean
operational_state_is_available (GDBusProxy *proxy)
{
  GVariant *v;
  const char *state;
  gboolean available;

  v = g_dbus_proxy_get_cached_property (proxy, "OperationalState");
  if (v == NULL)
    return FALSE;

  state = g_variant_get_string (v, NULL);
  available = (g_strcmp0 (state, "routable") == 0);
  g_variant_unref (v);

  return available;
}

static gboolean
set_availability (GNetworkMonitorSystemd *self,
                  gboolean                new_available,
                  GNetworkConnectivity    new_connectivity)
{
  gboolean changed = FALSE;

  g_object_freeze_notify (G_OBJECT (self));
  if (new_available != self->network_available)
    {
      self->network_available = new_available;
      g_object_notify (G_OBJECT (self), "network-available");
      changed = TRUE;
    }
  if (new_connectivity != self->connectivity)
    {
      self->connectivity = new_connectivity;
      g_object_notify (G_OBJECT (self), "connectivity");
      changed = TRUE;
    }
  g_object_thaw_notify (G_OBJECT (self));

  return changed;
}

static void
sync_properties (GNetworkMonitorSystemd *self)
{
  gboolean new_available;
  GNetworkConnectivity new_connectivity;

  new_available = operational_state_is_available (self->proxy);
  /* networkd performs no internet-reachability or captive-portal check, so we
   * can only distinguish "routable" (FULL) from "not routable" (LOCAL). */
  new_connectivity = new_available ? G_NETWORK_CONNECTIVITY_FULL
                                   : G_NETWORK_CONNECTIVITY_LOCAL;

  if (set_availability (self, new_available, new_connectivity))
    g_signal_emit_by_name (self, "network-changed", new_available);
}

static void
proxy_properties_changed_cb (GDBusProxy             *proxy,
                             GVariant               *changed_properties,
                             GStrv                   invalidated_properties,
                             GNetworkMonitorSystemd *self)
{
  sync_properties (self);
}

static gboolean finish_init (GNetworkMonitorSystemd  *self,
                             GDBusProxy              *proxy,
                             GError                 **error);
static void proxy_ready_cb (GObject      *source_object,
                            GAsyncResult *result,
                            gpointer      user_data);

/* networkd may be running without being the connectivity authority (e.g. it
 * manages no links). It then reports OnlineState "unknown". Only act as the
 * backend when we can confirm it is managing the network; otherwise decline so
 * a lower-priority backend (plain netlink) is used instead. */
static gboolean
networkd_is_authoritative (GDBusProxy *proxy)
{
  GVariant *v;
  const char *state;
  gboolean authoritative;

  v = g_dbus_proxy_get_cached_property (proxy, "OnlineState");
  if (v == NULL)
    return FALSE; /* networkd too old (pre-v249) to tell us; let netlink handle it. */

  state = g_variant_get_string (v, NULL);
  authoritative = g_strcmp0 (state, "unknown") != 0 && state[0] != '\0';
  g_variant_unref (v);

  return authoritative;
}

static gboolean
g_network_monitor_systemd_initable_init (GInitable     *initable,
                                         GCancellable  *cancellable,
                                         GError       **error)
{
  GNetworkMonitorSystemd *self = G_NETWORK_MONITOR_SYSTEMD (initable);
  GInitableIface *parent_iface;
  GDBusProxy *proxy = NULL;
  gboolean retval;

  /* Set up the netlink parent, which provides the route lookups used by
   * can_reach(). */
  parent_iface = g_type_interface_peek_parent (G_NETWORK_MONITOR_SYSTEMD_GET_INITABLE_IFACE (initable));
  if (!parent_iface->init (initable, cancellable, error))
    return FALSE;

  /* We need to determine whether networkd is usable. Ideally this would happen
   * asynchronously, but we need sync support for it for backwards compatibility.*/
  self->cancellable = g_cancellable_new ();
  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                         G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                         NULL,
                                         "org.freedesktop.network1",
                                         "/org/freedesktop/network1",
                                         "org.freedesktop.network1.Manager",
                                         self->cancellable,
                                         error);
  if (proxy == NULL)
    return FALSE;

  retval = finish_init (self, proxy, error);
  g_clear_object (&proxy);
  return retval;
}

static void
g_network_monitor_systemd_async_initable_init_async (GAsyncInitable      *initable,
                                                     int                  io_priority,
                                                     GCancellable        *cancellable,
                                                     GAsyncReadyCallback  callback,
                                                     void                *user_data)
{
  GNetworkMonitorSystemd *self = G_NETWORK_MONITOR_SYSTEMD (initable);
  GInitableIface *parent_iface;
  GTask *task = NULL;
  GError *local_error = NULL;

  task = g_task_new (initable, cancellable, callback, user_data);
  g_task_set_source_tag (task, g_network_monitor_systemd_async_initable_init_async);

  /* Set up the netlink parent, which provides the route lookups used by
   * can_reach(). We know this doesn’t implement GAsyncInitable, so no need to
   * try and do that asynchronously. */
  parent_iface = g_type_interface_peek_parent (G_NETWORK_MONITOR_SYSTEMD_GET_INITABLE_IFACE (initable));
  if (!parent_iface->init (G_INITABLE (initable), cancellable, &local_error))
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      g_clear_object (&task);
      return;
    }

  /* Whether networkd is usable is determined asynchronously in
   * proxy_ready_cb(), which holds a reference on the monitor. */
  self->cancellable = g_cancellable_new ();
  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                            G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                            NULL,
                            "org.freedesktop.network1",
                            "/org/freedesktop/network1",
                            "org.freedesktop.network1.Manager",
                            self->cancellable,
                            proxy_ready_cb,
                            g_steal_pointer (&task));
}

static void
proxy_ready_cb (GObject      *source_object,
                GAsyncResult *result,
                gpointer      user_data)
{
  GTask *task = g_steal_pointer (&user_data);
  GNetworkMonitorSystemd *self = g_task_get_source_object (task);
  GDBusProxy *proxy = NULL;
  GError *local_error = NULL;

  proxy = g_dbus_proxy_new_for_bus_finish (result, &local_error);

  if (proxy == NULL ||
      !finish_init (self, proxy, &local_error))
    g_task_return_error (task, g_steal_pointer (&local_error));
  else
    g_task_return_boolean (task, TRUE);

  g_clear_object (&proxy);
  g_clear_object (&task);
}

static gboolean
finish_init (GNetworkMonitorSystemd  *self,
             GDBusProxy              *proxy,
             GError                 **error)
{
  char *name_owner = NULL;
  gboolean retval;

  g_assert (proxy != NULL);

  name_owner = g_dbus_proxy_get_name_owner (proxy);

  if (name_owner == NULL ||
      !networkd_is_authoritative (proxy))
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           (name_owner == NULL)
                               ? "systemd-networkd is not running"
                               : "systemd-networkd is not managing the network");
      retval = FALSE;
    }
  else
    {
      self->signal_id = g_signal_connect (G_OBJECT (proxy), "g-properties-changed",
                                          G_CALLBACK (proxy_properties_changed_cb), self);
      self->proxy = g_steal_pointer (&proxy);
      sync_properties (self);
      retval = TRUE;
    }

  g_free (name_owner);

  return retval;
}

static gboolean
g_network_monitor_systemd_async_initable_init_finish (GAsyncInitable  *initable,
                                                      GAsyncResult    *result,
                                                      GError         **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
g_network_monitor_systemd_dispose (GObject *object)
{
  GNetworkMonitorSystemd *self = G_NETWORK_MONITOR_SYSTEMD (object);

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);

  g_clear_signal_handler (&self->signal_id, self->proxy);
  g_clear_object (&self->proxy);

  G_OBJECT_CLASS (g_network_monitor_systemd_parent_class)->dispose (object);
}

static void
g_network_monitor_systemd_class_init (GNetworkMonitorSystemdClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->dispose = g_network_monitor_systemd_dispose;
  gobject_class->get_property = g_network_monitor_systemd_get_property;

  g_object_class_override_property (gobject_class, PROP_NETWORK_AVAILABLE, "network-available");
  g_object_class_override_property (gobject_class, PROP_NETWORK_METERED, "network-metered");
  g_object_class_override_property (gobject_class, PROP_CONNECTIVITY, "connectivity");
}

static void
g_network_monitor_systemd_iface_init (GNetworkMonitorInterface *monitor_iface)
{
  /* can_reach() is inherited from the netlink backend. */
}

static void
g_network_monitor_systemd_initable_iface_init (GInitableIface *iface)
{
  iface->init = g_network_monitor_systemd_initable_init;
}

static void
g_network_monitor_systemd_async_initable_iface_init (GAsyncInitableIface *iface)
{
  iface->init_async = g_network_monitor_systemd_async_initable_init_async;
  iface->init_finish = g_network_monitor_systemd_async_initable_init_finish;
}
