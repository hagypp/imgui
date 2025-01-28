#pragma once
#include <windows.h>
#include <GL/GL.h>
#include <memory> // Add this include for std::unique_ptr
#include "movie_search_app.h"


struct WGL_WindowData { HDC hDC; };

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool init();
    void cleanup();
    bool processMessages(bool& done);
    void render();

    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    bool createDeviceWGL(HWND hWnd, WGL_WindowData* data);
    void cleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);

    HWND m_hwnd;
    HGLRC m_hRC;
    WGL_WindowData m_mainWindow;
    int m_width;
    int m_height;
    WNDCLASSEXW m_wc;
    std::unique_ptr<MovieSearchApp> m_app;
};