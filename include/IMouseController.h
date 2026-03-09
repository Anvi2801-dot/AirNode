#ifndef IMOUSECONTROLLER_H
#define IMOUSECONTROLLER_H

// Abstract Base Class (Interface)
class IMouseController {
public:
    virtual ~IMouseController() {}
    
    // Pure virtual functions (must be implemented by subclasses)
    virtual void move(int x, int y) = 0;
    virtual void click() = 0;
};

#endif