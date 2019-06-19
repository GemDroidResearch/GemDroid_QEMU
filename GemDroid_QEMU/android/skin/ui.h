/* Copyright (C) 2006-2010 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#ifndef ANDROID_SKIN_USER_INTERFACE_H
#define ANDROID_SKIN_USER_INTERFACE_H

#include "android/skin/rect.h"
#include "android/skin/keyboard.h"
#include "android/skin/keycode-buffer.h"

#include <stdbool.h>

typedef struct SkinUI SkinUI;

// Avoid including headers if possible.
struct SkinFile;
struct SkinLayout;
struct SkinTrackBallParameters;
struct SkinWindowFuncs;

// Function pointer used to toggle network emulation on/off.
// Returns the state of the network after the toggle, i.e. 'true' to
// indicate that it is enabled, 'false' otherwise.
typedef bool (*SkinUINetworkToggleFunc)(void);

// Function pointer used to indicate that the framebuffer display in the
// UI was lost and needs to be fully regenerated.
typedef void (*SkinUIInvalidateFrameBuffer)(void);

// A structure used to point to constant callbacks.
typedef struct SkinUIFuncs {
    const struct SkinWindowFuncs* window_funcs;
    const struct SkinTrackBallParameters* trackball_params;
    SkinKeyEventFunc keyboard_event;
    SkinKeyCodeFlushFunc keyboard_flush;
    SkinUINetworkToggleFunc network_toggle;
    SkinUIInvalidateFrameBuffer framebuffer_invalidate;
} SkinUIFuncs;

// A structure used to group initialization settings for a new SkinUI.
// |window_name| is a unique name for the window. Should typically be
// <port>:<name> where <port> is a console port number (e.g. 5554) and
// <name> is the AVD's name.
// |window_x| and |window_y| are the the initial UI window coordinates.
// |window_scale| is the window scaling factor. Use 1.0 for unscaled display.
// |enable_touch| is true iff touch emulation is needed.
// |enable_dpad| is true iff DPad emulation is needed.
// |enable_keyboard| is true iff a hardware keyboard must be emulated.
// |enable_trackball| is true iff a trackball must be emulated.
// |keyboard_charmap| is the name of the keyboard charmap, if any. If NULL,
// or empty, the default value will be used.
// |keyboard_raw_keys| is true to start in raw key input mode. Default is
// to start in Unicode mode. It's possible to toggle between modes at runtime
// by using Ctrl-K.
// |win32_ignore_events| is true to indicate that window events should be
// ignored on Windows.
typedef struct SkinUIParams {
    char window_name[64];
    int window_x;
    int window_y;
    double window_scale;

    bool enable_touch;
    bool enable_dpad;
    bool enable_keyboard;
    bool enable_trackball;

    const char* keyboard_charmap;
    bool keyboard_raw_keys;
    bool win32_ignore_events;

} SkinUIParams;

// Create a new user-interface SkinUI object
// |layout_file| is the original layout file for the corresponding skin.
// |initial_orientation| is the initial screen orientation, for example,
// "portrait" or "landscape".
// |keyboard| is the corresponding keyboard object, if any.
// |ui_funcs| points to a set of methods for interacting with the rest of the
// system.
// |ui_params| are initialization parameters. See SkinUIParams documentation.
//
// Note that |layout_file| and |keyboard| are still owned by the caller and
// must be freed by it. Of course, they must not be detroyed before the SkinUI
// object.
//
// |keyboard| will be ignored if |ui_params.enable_keyboard| is false. If
// it is NULL.
SkinUI* skin_ui_create(struct SkinFile* layout_file,
                       const char* initial_orientation,
                       const SkinUIFuncs* ui_funcs,
                       const SkinUIParams* params);

void skin_ui_set_onion(SkinUI* ui,
                       struct SkinImage* onion,
                       SkinRotation onion_rotation,
                       int onion_alpha);

void skin_ui_free(SkinUI* ui);

// Reset the name of the emulator window. This corresponds to the value
// of the |window_name| firled of SkinUIParams.
void skin_ui_set_name(SkinUI* ui, const char* name);

void skin_ui_set_lcd_brightness(SkinUI* ui, int lcd_brightness);

void skin_ui_set_scale(SkinUI* ui, double scale);

void skin_ui_update_display(SkinUI* ui, int x, int y, int w, int h);

void skin_ui_update_gpu_frame(SkinUI* ui, int w, int h, const void* pixels);

// Return the current SkinLayout used by the user interface.
struct SkinLayout* skin_ui_get_current_layout(SkinUI* ui);

// Process all pending user input events. Returns true if the program needs
// to exit/quit, false otherwise.
bool skin_ui_process_events(SkinUI* ui);

void skin_ui_reset_title(SkinUI* ui);

#endif  // ANDROID_SKIN_USER_INTERFACE_H
