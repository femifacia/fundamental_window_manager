/*
** EPITECH PROJECT, 2024
** window_manager
** File description:
** WindowManager
*/

#include "WindowManager.hpp"
bool WindowManager::_wmDetected;

std::unique_ptr<WindowManager> WindowManager::Create()
{
    Display *display = XOpenDisplay(nullptr);

    if (!display) {
        LOG(ERROR) << "failed to open X display " << XDisplayName(nullptr);
        return nullptr;
    }
    return std::unique_ptr<WindowManager>(new WindowManager(display));
}

WindowManager::WindowManager(Display* display):_display(CHECK_NOTNULL(display)), _root(DefaultRootWindow(_display))
{
}

WindowManager::~WindowManager()
{
    XCloseDisplay(_display);
}

void WindowManager::Run()
{
    _wmDetected = false;
    XSetErrorHandler(&WindowManager::OnWMDetected);
    XSelectInput(_display, _root, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(_display, false);
    if (_wmDetected) {
        LOG(ERROR) << "Detected another window Manager Running " << XDisplayString(_display);
        return;
    }
    XSetErrorHandler(&WindowManager::OnXError);

    XGrabServer(_display);
  //   d. Frame existing top-level windows.
  //     i. Query existing top-level windows.
  Window returned_root, returned_parent;
  Window* top_level_windows;
  unsigned int num_top_level_windows;
  CHECK(XQueryTree(
      _display,
      _root,
      &returned_root,
      &returned_parent,
      &top_level_windows,
      &num_top_level_windows));
  CHECK_EQ(returned_root, _root);
  //     ii. Frame each top-level window.
  for (unsigned int i = 0; i < num_top_level_windows; ++i) {
    Frame(top_level_windows[i], true /* was_created_before_window_manager */);
  }
  //     iii. Free top-level window array.
  XFree(top_level_windows);
  //   e. Ungrab X server.
  XUngrabServer(_display);

    XEvent e;
    while (1) {
        XNextEvent(_display, &e);
//        LOG(INFO) << "Received event: " << ToString(e);
        LOG(INFO) << "Received event" << std::endl;
        // 2. Dispatch event.
        switch (e.type) {
          case CreateNotify:
            OnCreateNotify(e.xcreatewindow);
            break;
          case DestroyNotify:
            OnDestroyNotify(e.xdestroywindow);
            break;
          case ReparentNotify:
            OnReparentNotify(e.xreparent);
            break;
          // etc. etc.
          case ConfigureRequest:
            OnConfigureRequest(e.xconfigurerequest);
            break;
          case MapRequest:
            OnMapRequest(e.xmaprequest);
            case UnmapNotify:
                OnUnmapNotify(e.xunmap);
          default:
            LOG(WARNING) << "Ignored event";
        }
    }
}

void WindowManager::OnUnmapNotify(XUnmapEvent &e)
{
    // If the window is a client window we manage, unframe it upon UnmapNotify. We
  // need the check because we will receive an UnmapNotify event for a frame
  // window we just destroyed ourselves.

    //indeed, when we unmap a window, we also unmap its frame so our window manager will also
    //receives again a UnMapNotification that we have to ignore. Thats why we only
    // unmap client windows

     // Ignore event if it is triggered by reparenting a window that was mapped
  // before the window manager started.
  //
  // Since we receive UnmapNotify events from the SubstructureNotify mask, the
  // event attribute specifies the parent window of the window that was
  // unmapped. This means that an UnmapNotify event from a normal client window
  // should have this attribute set to a frame window we maintain. Only an
  // UnmapNotify event triggered by reparenting a pre-existing window will have
  // this attribute set to the root window.
  if (e.event == _root) {
    LOG(INFO) << "Ignore UnmapNotify for reparented pre-existing window "
              << e.window;
    return;
  }

  Unframe(e.window);

    if (!_clients.count(e.window)) {
        LOG(INFO) << "Ignore UnmapNotify for non-client window " << e.window;
        return;
    }
    Unframe(e.window);
}

void WindowManager::Unframe(Window w)
{
    const Window frame = _clients[w];
  // 1. Unmap frame.
  XUnmapWindow(_display, frame);
  // 2. Reparent client window back to root window.
  XReparentWindow(
      _display,
      w,
      _root,
      0, 0);  // Offset of client window within root.
  // 3. Remove client window from save set, as it is now unrelated to us.
  XRemoveFromSaveSet(_display, w);
  // 4. Destroy frame.
  XDestroyWindow(_display, frame);
  // 5. Drop reference to frame handle.
  _clients.erase(w);

  LOG(INFO) << "Unframed window " << w << " [" << frame << "]";
}

void WindowManager::Frame(Window w, bool was_created_before_window_manager)
{

    const unsigned int BORDER_WIDTH = 8;
    const unsigned long BORDER_COLOR = 0xffb0a0;
    const unsigned long BG_COLOR = 0x10a0ff;
    XWindowAttributes x_window_attrs;
    CHECK(XGetWindowAttributes(_display, w, &x_window_attrs));

  // 2. TODO - see Framing Existing Top-Level Windows section below.
    if (was_created_before_window_manager) {
    if (x_window_attrs.override_redirect ||
        x_window_attrs.map_state != IsViewable) {
      return;
    }
  }

  // 3. Create frame.
    const Window frame = XCreateSimpleWindow(
        _display,
        _root,
        x_window_attrs.x,
        x_window_attrs.y,
        x_window_attrs.width,
        x_window_attrs.height,
        BORDER_WIDTH,
        BORDER_COLOR,
        BG_COLOR);
  // 3. Select events on frame.
    XSelectInput(
      _display,
      frame,
      SubstructureRedirectMask | SubstructureNotifyMask);
  // 4. Add client to save set, so that it will be restored and kept alive if we
  // crash.
  XAddToSaveSet(_display, w);
  // 5. Reparent client window.
  XReparentWindow(
        _display,
        w,
        frame,
        0, 0);  // Offset of client window within frame.
  // 6. Map frame.
    XMapWindow(_display, frame);
  // 7. Save frame handle.
    _clients[w] = frame;
  // 8. Grab events for window management actions on client window.
  //   a. Move windows with alt + left button.
  //XGrabButton(...);
  //   b. Resize windows with alt + right button.
  //XGrabButton(...);
  //   c. Kill windows with alt + f4.
  //XGrabKey(...);
  //   d. Switch windows with alt + tab.
  //XGrabKey(...);

    LOG(INFO) << "Framed window " << w << " [" << frame << "]";
}

void WindowManager::OnMapRequest(XMapRequestEvent &e)
{
    this->Frame(e.window, false);
    XMapWindow(_display, e.window);
}

void WindowManager::OnConfigureRequest(XConfigureRequestEvent &e)
{
    XWindowChanges changes;
    // Copy fields from e to changes.
    changes.x = e.x;
    changes.y = e.y;
    changes.width = e.width;
    changes.height = e.height;
    changes.border_width = e.border_width;
    changes.sibling = e.above;
    changes.stack_mode = e.detail;

    //when receiving configure request, we have to also reparenting the frame of the window concerned

    if (_clients.count(e.window)) {
        const Window frame = _clients[e.window];
        XConfigureWindow(_display, frame, e.value_mask, &changes);
        LOG(INFO) << "Resize [" << frame << "] to ";// << Size<int>(e.width, e.height);
    }

    // Grant request by calling XConfigureWindow().
    XConfigureWindow(_display, e.window, e.value_mask, &changes);
    LOG(INFO) << "Resize " << e.window << " to ";// << std::Size<int>(e.width, e.height);
}

void WindowManager::OnCreateNotify(XCreateWindowEvent &e)
{

}

void WindowManager::OnDestroyNotify(XDestroyWindowEvent &e)
{

}

void WindowManager::OnReparentNotify(XReparentEvent &e)
{

}

int WindowManager::OnWMDetected(Display *display, XErrorEvent *e)
{
    CHECK_EQ(static_cast<int>(e->error_code), BadAccess);
    _wmDetected = true;

    return 0;
}

int WindowManager::OnXError(Display *display, XErrorEvent *e)
{
    return 0;
}