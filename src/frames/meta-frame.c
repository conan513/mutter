/*
 * Copyright (C) 2022 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Carlos Garnacho <carlosg@gnome.org>
 */

#include "config.h"

#include "meta-frame.h"

#include "meta-frame-content.h"
#include "meta-frame-header.h"

#include <gdk/x11/gdkx.h>
#include <X11/Xatom.h>

struct _MetaFrame
{
  GtkWindow parent_instance;
  GtkWidget *content;
};

typedef struct
{
  unsigned long flags;
  unsigned long functions;
  unsigned long decorations;
  long input_mode;
  unsigned long status;
} MotifWmHints;

#define MWM_HINTS_FUNCTIONS (1L << 0)

#define MWM_FUNC_ALL (1L << 0)
#define MWM_FUNC_RESIZE (1L << 1)
#define MWM_FUNC_MINIMIZE (1L << 3)
#define MWM_FUNC_MAXIMIZE (1L << 4)
#define MWM_FUNC_CLOSE (1L << 5)

G_DEFINE_TYPE (MetaFrame, meta_frame, GTK_TYPE_WINDOW)

static void
meta_frame_class_init (MetaFrameClass *klass)
{
}

static gboolean
client_window_has_wm_protocol (MetaFrame *frame,
                               Window     client_window,
                               Atom       protocol)
{
  GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (frame));
  Atom *wm_protocols, wm_protocols_atom;
  int format;
  Atom type;
  unsigned long i, nitems, bytes_after;
  gboolean found = FALSE;

  gdk_x11_display_error_trap_push (display);

  wm_protocols_atom =
    gdk_x11_get_xatom_by_name_for_display (display, "WM_PROTOCOLS");

  if (XGetWindowProperty (gdk_x11_display_get_xdisplay (display),
                          client_window,
                          wm_protocols_atom,
                          0, G_MAXLONG, False,
                          XA_ATOM,
                          &type, &format,
                          &nitems, &bytes_after,
                          (unsigned char **) &wm_protocols) != Success)
    {
      gdk_x11_display_error_trap_pop_ignored (display);
      return FALSE;
    }

  if (gdk_x11_display_error_trap_pop (display))
    return FALSE;

  for (i = 0; i < nitems; i++)
    {
      if (wm_protocols[i] == protocol)
        {
          found = TRUE;
          break;
        }
    }

  XFree (wm_protocols);

  return found;
}

static gboolean
on_frame_close_request (GtkWindow *window,
                        gpointer   user_data)
{
  GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (window));
  GtkWidget *content;
  XClientMessageEvent ev;
  Window client_xwindow;
  Atom delete_window_atom;

  content = gtk_window_get_child (window);
  if (!content)
    return FALSE;

  client_xwindow =
    meta_frame_content_get_window (META_FRAME_CONTENT (content));

  delete_window_atom =
    gdk_x11_get_xatom_by_name_for_display (display, "WM_DELETE_WINDOW");

  gdk_x11_display_error_trap_push (display);

  if (client_window_has_wm_protocol (META_FRAME (window),
                                     client_xwindow,
                                     delete_window_atom))
    {
      ev.type = ClientMessage;
      ev.window = client_xwindow;
      ev.message_type =
        gdk_x11_get_xatom_by_name_for_display (display, "WM_PROTOCOLS");
      ev.format = 32;
      ev.data.l[0] = delete_window_atom;
      ev.data.l[1] = 0; /* FIXME: missing timestamp */

      XSendEvent (gdk_x11_display_get_xdisplay (display),
                  client_xwindow, False, 0, (XEvent*) &ev);
    }
  else
    {
      XKillClient (gdk_x11_display_get_xdisplay (display),
                   client_xwindow);
    }

  gdk_x11_display_error_trap_pop_ignored (display);

  return TRUE;
}

static void
meta_frame_init (MetaFrame *frame)
{
  g_signal_connect (frame, "close-request",
                    G_CALLBACK (on_frame_close_request), NULL);
}

static void
meta_frame_update_extents (MetaFrame *frame,
                           GtkBorder  border)
{
  GtkWindow *window = GTK_WINDOW (frame);
  GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (frame));
  GdkSurface *surface;
  Window xframe;
  unsigned long data[4];

  surface = gtk_native_get_surface (GTK_NATIVE (window));
  if (!surface)
    return;

  data[0] = border.left;
  data[1] = border.right;
  data[2] = border.top;
  data[3] = border.bottom;

  xframe = gdk_x11_surface_get_xid (surface);
  XChangeProperty (gdk_x11_display_get_xdisplay (display),
                   xframe,
                   gdk_x11_get_xatom_by_name_for_display (display, "_MUTTER_FRAME_EXTENTS"),
                   XA_CARDINAL,
                   32,
                   PropModeReplace,
                   (guchar *) &data, 4);
}

static void
on_border_changed (GObject    *object,
                   GParamSpec *pspec,
                   gpointer    user_data)
{
  MetaFrame *frame = user_data;
  GtkWidget *content;
  GtkBorder border;

  content = gtk_window_get_child (GTK_WINDOW (frame));
  border = meta_frame_content_get_border (META_FRAME_CONTENT (content));
  meta_frame_update_extents (frame, border);
}

static void
frame_sync_title (GtkWindow *frame,
                  Window     client_window)
{
  GdkDisplay *display;
  char *title = NULL;
  int format;
  Atom type;
  unsigned long nitems, bytes_after;

  display = gtk_widget_get_display (GTK_WIDGET (frame));

  XGetWindowProperty (gdk_x11_display_get_xdisplay (display),
                      client_window,
                      gdk_x11_get_xatom_by_name_for_display (display,
                                                             "_NET_WM_NAME"),
                      0, G_MAXLONG, False,
                      gdk_x11_get_xatom_by_name_for_display (display,
                                                             "UTF8_STRING"),
                      &type, &format,
                      &nitems, &bytes_after,
                      (unsigned char **) &title);

  gtk_window_set_title (frame, title);
  g_free (title);
}

static void
frame_sync_motif_wm_hints (GtkWindow *frame,
                           Window     client_window)
{
  GdkDisplay *display;
  MotifWmHints *mwm_hints = NULL;
  int format;
  Atom type;
  unsigned long nitems, bytes_after;
  gboolean deletable = TRUE;

  display = gtk_widget_get_display (GTK_WIDGET (frame));

  XGetWindowProperty (gdk_x11_display_get_xdisplay (display),
                      client_window,
                      gdk_x11_get_xatom_by_name_for_display (display,
                                                             "_MOTIF_WM_HINTS"),
                      0, sizeof (MotifWmHints) / sizeof (long),
                      False, AnyPropertyType,
                      &type, &format,
                      &nitems, &bytes_after,
                      (unsigned char **) &mwm_hints);

  if (mwm_hints &&
      (mwm_hints->flags & MWM_HINTS_FUNCTIONS) != 0)
    {
      if ((mwm_hints->functions & MWM_FUNC_ALL) == 0)
        deletable = (mwm_hints->functions & MWM_FUNC_CLOSE) != 0;
      else
        deletable = (mwm_hints->functions & MWM_FUNC_CLOSE) == 0;
    }

  gtk_window_set_deletable (frame, deletable);
  g_free (mwm_hints);
}

static void
frame_sync_wm_normal_hints (GtkWindow *frame,
                            Window     client_window)
{
  GdkDisplay *display;
  XSizeHints size_hints;
  long nitems;
  gboolean resizable = TRUE;

  display = gtk_widget_get_display (GTK_WIDGET (frame));

  XGetWMNormalHints (gdk_x11_display_get_xdisplay (display),
                     client_window,
                     &size_hints,
                     &nitems);

  if (nitems > 0)
    {
      resizable = ((size_hints.flags & PMinSize) == 0 ||
                   (size_hints.flags & PMaxSize) == 0 ||
                   size_hints.min_width != size_hints.max_width ||
                   size_hints.min_height != size_hints.max_height);
    }

  gtk_window_set_resizable (frame, resizable);
}

GtkWidget *
meta_frame_new (Window window)
{
  GtkWidget *frame, *header, *content;
  GdkSurface *surface;
  int frame_height;
  double scale;

  frame = g_object_new (META_TYPE_FRAME, NULL);

  header = meta_frame_header_new ();

  gtk_window_set_titlebar (GTK_WINDOW (frame), header);

  content = meta_frame_content_new (window);
  gtk_window_set_child (GTK_WINDOW (frame), content);

  g_signal_connect (content, "notify::border",
                    G_CALLBACK (on_border_changed), frame);

  gtk_widget_realize (GTK_WIDGET (frame));
  surface = gtk_native_get_surface (GTK_NATIVE (frame));
  gdk_x11_surface_set_frame_sync_enabled (surface, TRUE);

  gtk_widget_measure (header,
                      GTK_ORIENTATION_VERTICAL, 1,
                      &frame_height,
                      NULL, NULL, NULL);

  scale = gdk_surface_get_scale_factor (gtk_native_get_surface (GTK_NATIVE (frame)));

  meta_frame_update_extents (META_FRAME (frame),
                             (GtkBorder) {
                               0, 0,
                               frame_height * scale, 0,
                             });

  frame_sync_title (GTK_WINDOW (frame), window);
  frame_sync_motif_wm_hints (GTK_WINDOW (frame), window);
  frame_sync_wm_normal_hints (GTK_WINDOW (frame), window);

  return frame;
}

void
meta_frame_handle_xevent (MetaFrame *frame,
                          Window     window,
                          XEvent    *xevent)
{
  GdkDisplay *display;
  GtkWidget *content;
  gboolean is_frame, is_content;
  GdkSurface *surface;

  surface = gtk_native_get_surface (GTK_NATIVE (frame));
  if (!surface)
    return;

  content = gtk_window_get_child (GTK_WINDOW (frame));
  if (!content)
    return;

  is_frame = window == gdk_x11_surface_get_xid (surface);
  is_content =
    window == meta_frame_content_get_window (META_FRAME_CONTENT (content));

  if (!is_frame && !is_content)
    return;

  display = gtk_widget_get_display (GTK_WIDGET (frame));

  if (is_content && xevent->type == PropertyNotify)
    {
      if (xevent->xproperty.atom ==
          gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_NAME"))
        frame_sync_title (GTK_WINDOW (frame), xevent->xproperty.window);
      else if (xevent->xproperty.atom ==
               gdk_x11_get_xatom_by_name_for_display (display, "_MOTIF_WM_HINTS"))
        frame_sync_motif_wm_hints (GTK_WINDOW (frame), xevent->xproperty.window);
      else if (xevent->xproperty.atom ==
               gdk_x11_get_xatom_by_name_for_display (display, "WM_NORMAL_HINTS"))
        frame_sync_wm_normal_hints (GTK_WINDOW (frame), xevent->xproperty.window);
    }
}
