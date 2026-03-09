#include <ApplicationServices/ApplicationServices.h>
#include "../include/IMouseController.h"

class MacMouse : public IMouseController {
public:
    void move(int x, int y) override {
        CGPoint point = CGPointMake(x, y);
        CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, point, kCGMouseButtonLeft);
        CGEventPost(kCGHIDEventTap, e); CFRelease(e);
    }
    void click() override {
        CGEventRef ref = CGEventCreate(NULL);
        CGPoint pos = CGEventGetLocation(ref); CFRelease(ref);
        CGEventRef dn = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, pos, kCGMouseButtonLeft);
        CGEventRef up = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp,   pos, kCGMouseButtonLeft);
        CGEventPost(kCGHIDEventTap, dn); CGEventPost(kCGHIDEventTap, up);
        CFRelease(dn); CFRelease(up);
    }
    void scroll(float delta) override {
        CGEventRef e = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitLine, 1, (int32_t)delta);
        CGEventPost(kCGHIDEventTap, e); CFRelease(e);
    }
};

IMouseController* createMacMouse() { return new MacMouse(); }
