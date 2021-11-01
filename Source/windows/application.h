#pragma once

class CApplication {

    HWND hWnd;
    HINSTANCE hInstance;

    int  width = 1280;
    int  height = 720;
    static CApplication* the_app;

    bool mouse_visible = true;
    bool mouse_centered = false;
    bool should_exit = false;

public:

    bool init(HINSTANCE hInstance, int width, int height);
    void run();
    HWND getHandle() {
        return hWnd;
    }

    HINSTANCE getHInstance() {
        return hInstance;
    }

    void getDimensions(int& awidth, int& aheight) const;
    void setDimensions(int awidth, int aheight);

    void setMouseVisible(bool hidden);
    void setMouseCentered(bool centered);
    bool getMouseHidden();
    bool getMouseCentered();
    void exit() { should_exit = true; };

    // State: Show (True) or Hide (false)
    void changeMouseState(bool state, bool center = true);

    static CApplication& get();
};
