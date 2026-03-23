#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include "../include/IMouseController.h"

class MacMouse : public IMouseController {
public:
    void move(int x, int y) override {
        CGPoint p = CGPointMake(x, y);
        CGEventType type = _dragging ? kCGEventLeftMouseDragged : kCGEventMouseMoved;
        CGEventRef e = CGEventCreateMouseEvent(NULL, type, p, kCGMouseButtonLeft);
        CGEventPost(kCGHIDEventTap, e); CFRelease(e);
        _lastPos = p;
    }

    void click() override {
        CGEventRef dn = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, _lastPos, kCGMouseButtonLeft);
        CGEventRef up = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp,   _lastPos, kCGMouseButtonLeft);
        CGEventSetIntegerValueField(dn, kCGMouseEventClickState, 1);
        CGEventSetIntegerValueField(up, kCGMouseEventClickState, 1);
        CGEventPost(kCGHIDEventTap, dn); CGEventPost(kCGHIDEventTap, up);
        CFRelease(dn); CFRelease(up);
    }

    void doubleClick() override {
        CGEventRef dn1 = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, _lastPos, kCGMouseButtonLeft);
        CGEventRef up1 = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp,   _lastPos, kCGMouseButtonLeft);
        CGEventSetIntegerValueField(dn1, kCGMouseEventClickState, 1);
        CGEventSetIntegerValueField(up1, kCGMouseEventClickState, 1);
        CGEventPost(kCGHIDEventTap, dn1); CGEventPost(kCGHIDEventTap, up1);
        CFRelease(dn1); CFRelease(up1);

        CGEventRef dn2 = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, _lastPos, kCGMouseButtonLeft);
        CGEventRef up2 = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp,   _lastPos, kCGMouseButtonLeft);
        CGEventSetIntegerValueField(dn2, kCGMouseEventClickState, 2);
        CGEventSetIntegerValueField(up2, kCGMouseEventClickState, 2);
        CGEventPost(kCGHIDEventTap, dn2); CGEventPost(kCGHIDEventTap, up2);
        CFRelease(dn2); CFRelease(up2);
    }

    void rightClick() override {
        CGEventRef dn = CGEventCreateMouseEvent(NULL, kCGEventRightMouseDown, _lastPos, kCGMouseButtonRight);
        CGEventRef up = CGEventCreateMouseEvent(NULL, kCGEventRightMouseUp,   _lastPos, kCGMouseButtonRight);
        CGEventPost(kCGHIDEventTap, dn); CGEventPost(kCGHIDEventTap, up);
        CFRelease(dn); CFRelease(up);
    }

    void dragStart(int x, int y) override {
        _lastPos  = CGPointMake(x, y);
        _dragging = true;
        CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, _lastPos, kCGMouseButtonLeft);
        CGEventSetIntegerValueField(e, kCGMouseEventClickState, 1);
        CGEventPost(kCGHIDEventTap, e); CFRelease(e);
    }

    void dragEnd(int x, int y) override {
        _lastPos  = CGPointMake(x, y);
        _dragging = false;
        CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, _lastPos, kCGMouseButtonLeft);
        CGEventSetIntegerValueField(e, kCGMouseEventClickState, 1);
        CGEventPost(kCGHIDEventTap, e); CFRelease(e);
    }

    void scroll(float delta) override {
        CGEventRef e = CGEventCreateScrollWheelEvent(
            NULL, kCGScrollEventUnitLine, 1, (int32_t)delta);
        CGEventPost(kCGHIDEventTap, e); CFRelease(e);
    }

    void swipe(bool leftToRight) override {
        // Three-finger swipe: Mission Control / App Expose via CGEvent
        CGEventRef e = CGEventCreate(NULL);
        // Post Ctrl+Up for Mission Control, Ctrl+Down for App Expose
        CGKeyCode key = leftToRight ? 125 : 126; // down/up arrow
        CGEventRef keyDown = CGEventCreateKeyboardEvent(NULL, key, true);
        CGEventRef keyUp   = CGEventCreateKeyboardEvent(NULL, key, false);
        CGEventSetFlags(keyDown, kCGEventFlagMaskControl);
        CGEventSetFlags(keyUp,   kCGEventFlagMaskControl);
        CGEventPost(kCGHIDEventTap, keyDown);
        CGEventPost(kCGHIDEventTap, keyUp);
        CFRelease(keyDown); CFRelease(keyUp); CFRelease(e);
    }

    void zoom(float delta) override {
        // Cmd + for zoom in, Cmd - for zoom out
        CGKeyCode key = (delta > 0) ? kVK_ANSI_Equal : kVK_ANSI_Minus;
        CGEventRef dn = CGEventCreateKeyboardEvent(NULL, key, true);
        CGEventRef up = CGEventCreateKeyboardEvent(NULL, key, false);
        CGEventSetFlags(dn, kCGEventFlagMaskCommand);
        CGEventSetFlags(up, kCGEventFlagMaskCommand);
        CGEventPost(kCGHIDEventTap, dn); CGEventPost(kCGHIDEventTap, up);
        CFRelease(dn); CFRelease(up);
    }

private:
    CGPoint _lastPos  = CGPointMake(0, 0);
    bool    _dragging = false;
};

IMouseController* createMacMouse() { return new MacMouse(); }