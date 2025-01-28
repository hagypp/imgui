#include "main_window.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"
#include "movie_search_app.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Store the MainWindow instance pointer for the static WndProc
static MainWindow* g_MainWindowInstance = nullptr;

MainWindow::MainWindow()
    : m_hwnd(nullptr), m_hRC(nullptr), m_width(1280), m_height(800) {
    m_app = std::make_unique<MovieSearchApp>();
    g_MainWindowInstance = this;  // Store instance for WndProc
}

MainWindow::~MainWindow() {
    cleanup();
    g_MainWindowInstance = nullptr;
}

bool MainWindow::init() {
    m_wc = {
        sizeof(m_wc), CS_OWNDC,
        WndProc, 0L, 0L,
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
        L"ImGui Movie Search", nullptr
    };

    ::RegisterClassExW(&m_wc);
    m_hwnd = ::CreateWindowW(
        m_wc.lpszClassName,
        L"IMDB Movie Search",
        WS_OVERLAPPEDWINDOW,
        100, 100, m_width, m_height,
        nullptr, nullptr,
        m_wc.hInstance,
        nullptr
    );

    if (!createDeviceWGL(m_hwnd, &m_mainWindow)) {
        cleanupDeviceWGL(m_hwnd, &m_mainWindow);
        ::DestroyWindow(m_hwnd);
        ::UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);
        return false;
    }

    wglMakeCurrent(m_mainWindow.hDC, m_hRC);

    ::ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(m_hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_InitForOpenGL(m_hwnd);
    ImGui_ImplOpenGL3_Init();

    return true;
}

void MainWindow::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    cleanupDeviceWGL(m_hwnd, &m_mainWindow);
    wglDeleteContext(m_hRC);
    ::DestroyWindow(m_hwnd);
    ::UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);
}

bool MainWindow::processMessages(bool& done) {
    MSG msg;
    while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT) {
            done = true;
            return false;
        }
    }

    if (::IsIconic(m_hwnd)) {
        ::Sleep(10);
        return false;
    }

    return true;
}

void MainWindow::render() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    m_app->render();    //call for the render function in the app

    ImGui::Render();
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ::SwapBuffers(m_mainWindow.hDC);
}

bool MainWindow::createDeviceWGL(HWND hWnd, WGL_WindowData* data) {
    HDC hDc = ::GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    const int pf = ::ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;
    ::ReleaseDC(hWnd, hDc);

    data->hDC = ::GetDC(hWnd);
    if (!m_hRC)
        m_hRC = wglCreateContext(data->hDC);
    return true;
}

void MainWindow::cleanupDeviceWGL(HWND hWnd, WGL_WindowData* data) {
    wglMakeCurrent(nullptr, nullptr);
    ::ReleaseDC(hWnd, data->hDC);
}

LRESULT WINAPI MainWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    if (g_MainWindowInstance != nullptr) {
        switch (msg) {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                g_MainWindowInstance->m_width = LOWORD(lParam);
                g_MainWindowInstance->m_height = HIWORD(lParam);
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU)
                return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        }
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}