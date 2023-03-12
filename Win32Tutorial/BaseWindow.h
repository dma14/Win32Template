#pragma once
#include <windows.h>

// Note: template classes need to be all in the header, not split between .h and .cpp.
// Otherwise there's some issues in the linker...

template <class DERIVED_TYPE>
class BaseWindow
{
public:
	// Empty constructor, initialize m_hwnd to NULL
	BaseWindow() : m_hwnd(NULL) {}
    // Function to get the window handle for this window
    HWND Window() const { return m_hwnd; }

	// Static WindowProc function, which invokes overridden HandleMessage()
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        // This is the pointer to "this" object, which will store the state
        DERIVED_TYPE* pThis = NULL;

        // In this static WindowProc, we just need to handle 
        if (uMsg == WM_NCCREATE) {
            // WM_NCCREATE is called first when a window is created

            // CREATESTRUCT is pointed to by lParam, get it
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            // CREATESTRUCT has a pointer to the user data, get it
            pThis = reinterpret_cast<DERIVED_TYPE*>(pCreate->lpCreateParams);
            // Now that we have the pointer to "this", store it in the window as user data
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

            // Also need to set m_hwnd, I'm not actually sure why...
            pThis->m_hwnd = hwnd;
        } else {
            // Once the WindowLongPtr has been set up once, we can just get it
            pThis = (DERIVED_TYPE*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        // If we have a properly set up inheriting object window, call its HandleMessage(), 
        // otherwise go to default
        if (pThis) {
            return pThis->HandleMessage(uMsg, wParam, lParam);
        } else {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        };
    };

    // Create the window, shouldn't have to override this
    BOOL Create(
        PCWSTR lpWindowName,
        DWORD dwStyle,
        DWORD dwExStyle = 0,
        int x = CW_USEDEFAULT,
        int y = CW_USEDEFAULT,
        int nWidth = CW_USEDEFAULT,
        int nHeight = CW_USEDEFAULT,
        HWND hWndParent = 0,
        HMENU hMenu = 0
    ) {
        // Default options are all 0 for the class
        WNDCLASS wc = { 0 };

        // Set the window process to this object's (the inheriting class) WindowProc function.
        // It needs to be a static member function so that we can get the pointer.
        wc.lpfnWndProc = DERIVED_TYPE::WindowProc;
        // By passing NULL into GetModuleHandle(), we get the handle of the file used to create the calling process,
        // which should be the same as getting it from wWinMain()?
        wc.hInstance = GetModuleHandle(NULL);
        // Let the inheriting class define the class name
        wc.lpszClassName = ClassName();

        // Register our wndclass with the OS
        RegisterClass(&wc);

        // Create our window, notably pass "this" into the additional application data field (lParam)
        // Most args here are passed directly from the args of this function.
        m_hwnd = CreateWindowEx(
            dwExStyle, ClassName(), lpWindowName, dwStyle, x, y,
            nWidth, nHeight, hWndParent, hMenu, GetModuleHandle(NULL), this
        );

        // Return TRUE if we were able to create the window
        return (m_hwnd ? TRUE : FALSE);
    };

protected:
    virtual PCWSTR ClassName() const = 0;
    virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
    HWND m_hwnd;
};