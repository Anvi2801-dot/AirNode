#ifndef MOUSEFACTORY_H
#define MOUSEFACTORY_H

#include "IMouseController.h"

#ifdef __APPLE__
    IMouseController* createMacMouse();
#elif _WIN32
    IMouseController* createWindowsMouse();
#endif

class MouseFactory {
public:
    static IMouseController* createMouse() {
#ifdef __APPLE__
        return createMacMouse();
#elif _WIN32
        return createWindowsMouse();
#else
        return nullptr;
#endif
    }
};

#endif
