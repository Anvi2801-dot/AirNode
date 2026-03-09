#include <ApplicationServices/ApplicationServices.h>
#include "../include/IMouseController.h"

class MacMouse : public IMouseController {
public:
    void move(int x, int y) override {
        CGPoint point = CGPointMake(static_cast<CGFloat>(x), static_cast<CGFloat>(y));
        CGEventRef moveEvent = CGEventCreateMouseEvent(
            NULL, kCGEventMouseMoved, point, kCGMouseButtonLeft
        );
        CGEventPost(kCGHIDEventTap, moveEvent);
        CFRelease(moveEvent);
    }

    void click() override {
        // Clicks require a Down event followed by an Up event
        CGEventRef ourEvent = CGEventCreate(NULL);
        CGPoint currentPos = CGEventGetLocation(ourEvent);
        CFRelease(ourEvent);

        CGEventRef clickDown = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, currentPos, kCGMouseButtonLeft);
        CGEventRef clickUp = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, currentPos, kCGMouseButtonLeft);
        
        CGEventPost(kCGHIDEventTap, clickDown);
        CGEventPost(kCGHIDEventTap, clickUp);
        
        CFRelease(clickDown);
        CFRelease(clickUp);
    }
};

// Add this at the bottom of src/MacMouse.cpp
IMouseController* createMacMouse() {
    return new MacMouse();
}