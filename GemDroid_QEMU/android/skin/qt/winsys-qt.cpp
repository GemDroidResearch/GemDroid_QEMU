/* Copyright (C) 2014 The Android Open Source Project
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
#include <stdio.h>
#ifdef CONFIG_POSIX
#include <pthread.h>
#endif

#include "android/skin/rect.h"
#include "android/skin/resource.h"
#include "android/skin/winsys.h"
#include "android/skin/qt/emulator-window.h"
#include "android/utils/setenv.h"

#include <QtCore>
#include <QApplication>
#include <QDesktopWidget>
#include <QRect>
#include <QSemaphore>
#include <QThread>
#include <QWidget>

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"

#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

struct GlobalState {
    int argc;
    char** argv;
    QApplication* app;
    bool window_pos_saved;
    int window_pos_x;
    int window_pos_y;
};

static GlobalState* globalState() {
    static GlobalState sGlobalState = {
        .argc = 0,
        .argv = NULL,
        .app = NULL,
        .window_pos_saved = false,
        .window_pos_x = 0,
        .window_pos_y = 0,
    };
    return &sGlobalState;
}

//static char **static_argv;
//static int static_argc;
//static QApplication *app = NULL;

extern void skin_winsys_enter_main_loop(int argc, char **argv)
{
    D("Starting QT main loop\n");
    GlobalState* g = globalState();
    g->argc = argc;
    g->argv = argv;
    g->app->exec();
    D("Finished QT main loop\n");
    exit(0);
}

extern void skin_winsys_get_monitor_rect(SkinRect *rect)
{
    QRect qrect;
    QSemaphore semaphore;
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->getScreenDimensions(&qrect, &semaphore);
    semaphore.acquire();
    rect->pos.x = qrect.left();
    rect->pos.y = qrect.top();
    rect->size.w = qrect.width();
    rect->size.h = qrect.height();
    D("%s: (%d,%d) %dx%d", __FUNCTION__, rect->pos.x, rect->pos.y,
      rect->size.w, rect->size.h);
}

extern int skin_winsys_get_monitor_dpi(int *x, int *y)
{
    D("skin_winsys_get_monitor_dpi");
    QSemaphore semaphore;
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return -1;
    }
    int value;
    window->getMonitorDpi(&value, &semaphore);
    semaphore.acquire();
    *x = *y = value;
    D("%s: result=%d", __FUNCTION__, value);
    return 0;
}

extern void *skin_winsys_get_window_handle(void)
{
    D("skin_winsys_get_window_handle");
    WId handle;
    QSemaphore semaphore;
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return NULL;
    }
    window->getWindowId(&handle, &semaphore);
    semaphore.acquire();
    D("%s: result = 0x%p", __FUNCTION__, (void*)handle);
    return (void*)handle;
}

extern void skin_winsys_get_window_pos(int *x, int *y)
{
    D("skin_winsys_get_window_pos");
    GlobalState* g = globalState();
    if (g->window_pos_saved) {
        *x = g->window_pos_x;
        *y = g->window_pos_y;
    } else {
        QSemaphore semaphore;
        EmulatorWindow *window = EmulatorWindow::getInstance();
        if (window == NULL) {
            D("%s: Could not get window handle", __FUNCTION__);
            return;
        }
        window->getWindowPos(x, y, &semaphore);
        semaphore.acquire();
    }
    D("%s: x=%d y=%d", __FUNCTION__, *x, *y);
}

extern void skin_winsys_save_window_pos() {
    int x = 0, y = 0;
    skin_winsys_get_window_pos(&x, &y);
    GlobalState* g = globalState();
    g->window_pos_saved = true;
    g->window_pos_x = x;
    g->window_pos_y = y;
}

extern bool skin_winsys_is_window_fully_visible()
{
    D("skin_winsys_is_window_fully_visible");
    QSemaphore semaphore;
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return true;
    }
    bool value;
    window->isWindowFullyVisible(&value, &semaphore);
    semaphore.acquire();
    D("%s: result = %s", __FUNCTION__, value ? "true" : "false");
    return value;
}

extern void skin_winsys_quit()
{
    D("skin_winsys_quit");
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->requestClose();
}

extern void skin_winsys_set_relative_mouse_mode(bool)
{
    D("skin_winsys_set_relative_mouse_mode");
}

extern void skin_winsys_set_window_icon(const unsigned char *data, size_t size)
{
    D("skin_winsys_set_window_icon");
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->setWindowIcon(data, size);
}

extern void skin_winsys_set_window_pos(int x, int y)
{
    D("skin_winsys_set_window_pos %d, %d", x, y);
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->setWindowPos(x, y);
}

extern void skin_winsys_set_window_title(const char *title)
{
    D("skin_winsys_set_window_title [%s]", title);
    QSemaphore semaphore;
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    QString qtitle(title);
    window->setTitle(&qtitle, &semaphore);
    semaphore.acquire();
}

extern void skin_winsys_spawn_thread(StartFunction f, int argc, char **argv)
{
    D("skin_spawn_thread");
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->startThread(f, argc, argv);
}

extern void skin_winsys_start(bool, bool)
{
    GlobalState* g = globalState();
    g->app = new QApplication(g->argc, g->argv);
    new EmulatorWindow(NULL);
}

#ifdef _WIN32
extern "C" int qt_main(int, char**);

int qMain(int argc, char** argv) {
    return qt_main(argc, argv);
}
#endif  // _WIN32

