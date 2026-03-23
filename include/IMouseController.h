#ifndef IMOUSECONTROLLER_H
#define IMOUSECONTROLLER_H

class IMouseController {
public:
    virtual ~IMouseController() {}
    virtual void move(int x, int y)      = 0;
    virtual void click()                 = 0;
    virtual void doubleClick()           = 0;
    virtual void rightClick()            = 0;
    virtual void dragStart(int x, int y) = 0;
    virtual void dragEnd(int x, int y)   = 0;
    virtual void scroll(float delta)     = 0;
    virtual void swipe(bool leftToRight) = 0;
    virtual void zoom(float delta)       = 0;
};

#endif