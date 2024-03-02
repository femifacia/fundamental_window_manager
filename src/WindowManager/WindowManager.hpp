/*
** EPITECH PROJECT, 2024
** window_manager
** File description:
** WindowManager
*/

#ifndef WINDOWMANAGER_HPP_
#define WINDOWMANAGER_HPP_

#include <memory>
#include <glog/logging.h>
extern "C" {
    #include <X11/Xlib.h>
} 
#include <iostream>
#include <string>
#include <cstring>  
#include <unordered_map>

class WindowManager {
    public:

        static std::unique_ptr<WindowManager> Create();
        void Run();
        ~WindowManager();

        static int OnXError(Display *display, XErrorEvent *e);

        static int OnWMDetected(Display *display, XErrorEvent *e);

        static bool _wmDetected;


    protected:
        WindowManager(Display *display);
        Display *_display;
        const Window _root;

    private:
        void OnCreateNotify(XCreateWindowEvent &e);
        void OnDestroyNotify(XDestroyWindowEvent &e);
        void OnReparentNotify(XReparentEvent &e);
        void OnConfigureRequest(XConfigureRequestEvent &e);
        void OnMapRequest(XMapRequestEvent &e);
        void OnUnmapNotify(XUnmapEvent &e);
        void Frame(Window w, bool was_created_before_wm);
        void Unframe(Window w);

        std::unordered_map<Window, Window> _clients;

};

#endif /* !WINDOWMANAGER_HPP_ */
