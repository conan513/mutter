/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright 2014 Red Hat, Inc.
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

#ifndef META_INPUT_SETTINGS_PRIVATE_H
#define META_INPUT_SETTINGS_PRIVATE_H

#include <gdesktop-enums.h>

#ifdef HAVE_LIBWACOM
#include <libwacom/libwacom.h>
#endif

#include "backends/meta-backend-types.h"
#include "clutter/clutter.h"
#include "meta/display.h"
#include "meta/meta-enums.h"

#define META_TYPE_INPUT_SETTINGS (meta_input_settings_get_type ())
G_DECLARE_DERIVABLE_TYPE (MetaInputSettings, meta_input_settings,
                          META, INPUT_SETTINGS, GObject)

/**
 * MetaKbdA11ySettings:
 *
 * The #MetaKbdA11ySettings structure contains keyboard accessibility
 * settings
 *
 */
typedef struct _MetaKbdA11ySettings
{
  MetaKeyboardA11yFlags controls;
  int slowkeys_delay;
  int debounce_delay;
  int timeout_delay;
  int mousekeys_init_delay;
  int mousekeys_max_speed;
  int mousekeys_accel_time;
} MetaKbdA11ySettings;

struct _MetaInputSettingsClass
{
  GObjectClass parent_class;

  void (* set_send_events)   (MetaInputSettings        *settings,
                              ClutterInputDevice       *device,
                              GDesktopDeviceSendEvents  mode);
  void (* set_matrix)        (MetaInputSettings  *settings,
                              ClutterInputDevice *device,
                              const float         matrix[6]);
  void (* set_speed)         (MetaInputSettings  *settings,
                              ClutterInputDevice *device,
                              gdouble             speed);
  void (* set_left_handed)   (MetaInputSettings  *settings,
                              ClutterInputDevice *device,
                              gboolean            enabled);
  void (* set_tap_enabled)   (MetaInputSettings  *settings,
                              ClutterInputDevice *device,
                              gboolean            enabled);
  void (* set_tap_button_map) (MetaInputSettings            *settings,
                               ClutterInputDevice           *device,
                               GDesktopTouchpadTapButtonMap  mode);
  void (* set_tap_and_drag_enabled) (MetaInputSettings  *settings,
                                     ClutterInputDevice *device,
                                     gboolean            enabled);
  void (* set_tap_and_drag_lock_enabled) (MetaInputSettings  *settings,
                                          ClutterInputDevice *device,
                                          gboolean            enabled);
  void (* set_disable_while_typing) (MetaInputSettings  *settings,
                                     ClutterInputDevice *device,
                                     gboolean            enabled);
  void (* set_invert_scroll) (MetaInputSettings  *settings,
                              ClutterInputDevice *device,
                              gboolean            inverted);
  void (* set_edge_scroll)   (MetaInputSettings  *settings,
                              ClutterInputDevice *device,
                              gboolean            enabled);
  void (* set_two_finger_scroll) (MetaInputSettings  *settings,
                                  ClutterInputDevice *device,
                                  gboolean            enabled);
  void (* set_scroll_button) (MetaInputSettings  *settings,
                              ClutterInputDevice *device,
                              guint               button,
                              gboolean            button_lock);

  void (* set_click_method)  (MetaInputSettings            *settings,
                              ClutterInputDevice           *device,
                              GDesktopTouchpadClickMethod   mode);

  void (* set_keyboard_repeat) (MetaInputSettings *settings,
                                gboolean           repeat,
                                guint              delay,
                                guint              interval);

  void (* set_tablet_mapping)        (MetaInputSettings      *settings,
                                      ClutterInputDevice     *device,
                                      GDesktopTabletMapping   mapping);
  void (* set_tablet_aspect_ratio)   (MetaInputSettings      *settings,
                                      ClutterInputDevice     *device,
                                      double                  ratio);
  void (* set_tablet_area)           (MetaInputSettings      *settings,
                                      ClutterInputDevice     *device,
                                      gdouble                 padding_left,
                                      gdouble                 padding_right,
                                      gdouble                 padding_top,
                                      gdouble                 padding_bottom);

  void (* set_mouse_accel_profile) (MetaInputSettings          *settings,
                                    ClutterInputDevice         *device,
                                    GDesktopPointerAccelProfile profile);
  void (* set_trackball_accel_profile) (MetaInputSettings          *settings,
                                        ClutterInputDevice         *device,
                                        GDesktopPointerAccelProfile profile);

  void (* set_stylus_pressure) (MetaInputSettings            *settings,
                                ClutterInputDevice           *device,
                                ClutterInputDeviceTool       *tool,
                                const gint32                  curve[4]);
  void (* set_stylus_button_map) (MetaInputSettings          *settings,
                                  ClutterInputDevice         *device,
                                  ClutterInputDeviceTool     *tool,
                                  GDesktopStylusButtonAction  primary,
                                  GDesktopStylusButtonAction  secondary,
                                  GDesktopStylusButtonAction  tertiary);

  void (* set_mouse_middle_click_emulation) (MetaInputSettings  *settings,
                                             ClutterInputDevice *device,
                                             gboolean            enabled);
  void (* set_touchpad_middle_click_emulation) (MetaInputSettings  *settings,
                                                ClutterInputDevice *device,
                                                gboolean            enabled);
  void (* set_trackball_middle_click_emulation) (MetaInputSettings  *settings,
                                                 ClutterInputDevice *device,
                                                 gboolean            enabled);

  gboolean (* has_two_finger_scroll) (MetaInputSettings  *settings,
                                      ClutterInputDevice *device);
};

void meta_input_settings_maybe_save_numlock_state (MetaInputSettings *input_settings,
                                                   gboolean           numlock_state);
gboolean meta_input_settings_maybe_restore_numlock_state (MetaInputSettings *input_settings);

void meta_input_settings_set_device_matrix (MetaInputSettings  *input_settings,
                                            ClutterInputDevice *device,
                                            float               matrix[6]);
void meta_input_settings_set_device_enabled (MetaInputSettings  *input_settings,
                                             ClutterInputDevice *device,
                                             gboolean            enabled);
void meta_input_settings_set_device_aspect_ratio (MetaInputSettings  *input_settings,
                                                  ClutterInputDevice *device,
                                                  double              aspect_ratio);

void meta_input_settings_get_kbd_a11y_settings (MetaInputSettings   *input_settings,
                                                MetaKbdA11ySettings *a11y_settings);

void meta_input_settings_add_device (MetaInputSettings  *input_settings,
                                     ClutterInputDevice *device);
void meta_input_settings_remove_device (MetaInputSettings  *input_settings,
                                        ClutterInputDevice *device);
void meta_input_settings_notify_tool_change (MetaInputSettings      *input_settings,
                                             ClutterInputDevice     *device,
                                             ClutterInputDeviceTool *tool);
void meta_input_settings_notify_kbd_a11y_change (MetaInputSettings     *input_settings,
                                                 MetaKeyboardA11yFlags  new_flags,
                                                 MetaKeyboardA11yFlags  what_changed);

#endif /* META_INPUT_SETTINGS_PRIVATE_H */
