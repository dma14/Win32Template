#include "MainWindow.h"
#include "BaseWindow.h"

// Special safe release function for releasing COM interface pointers
template <class T> void SafeRelease(T** ppT)
{
    if (*ppT) {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_CREATE:
    {
        // Initialize a timer for our graphics refresh rate
        SetTimer(m_hwnd, timerId, 10, (TIMERPROC)NULL);
        // On window creation, initialize the Direct2D factory
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory))) {
            return -1;
        }
        // Also initialize a DirectWrite factory for writing text.
        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(pWriteFactory),
            reinterpret_cast<IUnknown**>(&pWriteFactory)))) {
            return -1;
        }
        // Use the WriteFactory to create a DirectWrite text format object
        if (FAILED(pWriteFactory->CreateTextFormat(
            L"Verdana",
            NULL,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            50,
            L"", //locale
            &pTextFormat))) {
            return -1;
        }
        // Now align the text
        pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        return 0;
    }

    case WM_DESTROY:
    {
        // Release our graphics resources
        KillTimer(m_hwnd, timerId);
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        return 0;
    }

    case WM_PAINT:
    {
        // WM_PAINT is sent when we need to paint a portion of the app window, and
        // also when UpdateWindow() and RedrawWindow() are called.
        OnPaint();
        return 0;
    }

    case WM_SIZE:
    {
        // WM_SIZE is invoked when the window changes size
        Resize();
        return 0;
    }

    case WM_TIMER:
    {
        if (wParam == timerId) {
            timeElapsed++;
            // Uncomment this to use the timer to trigger repainting (it doesn't actually fire every 10ms)
            OnPaint();
        }
        return 0;
    }

    default:
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }
    return TRUE;
}

HRESULT MainWindow::CreateGraphicsResources() {
    HRESULT hr = S_OK;

    // Only run if the render target isn't already created
    if (pRenderTarget == NULL) {
        // Get the rectangle (the struct is just coordinates) for the window's client area
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        // Now use the factory to create the render target, default target properties, and pass the window 
        // handle + size in
        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);

        // If that worked, use the render target to create the brush
        // We use the render target instead of the factory since render target handles device dependent resources.
        if (SUCCEEDED(hr)) {
            // Choose a color for the brush (red, green, blue, alpha) -> yellow
            // See: https://docs.microsoft.com/en-us/windows/win32/learnwin32/using-color-in-direct2d
            const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);

            // If both worked, we should update our layout (the shapes we're drawing)
            if (SUCCEEDED(hr)) {
                CalculateLayout();
            }
        }
    }

    return hr;
}

void MainWindow::DiscardGraphicsResources()
{
    // Use this special SafeRelease function
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

void MainWindow::OnPaint() {
    // The standard Direct2D render loop is: https://docs.microsoft.com/en-us/windows/win32/learnwin32/drawing-with-direct2d#the-direct2d-render-loop
    // Invoke CreateGraphicsResources() so that we can start painting
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr)) {
        // Start painting
        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        // In Direct2D, drawing is done on the render target
        // Start Direct2D drawing
        pRenderTarget->BeginDraw();

        // Clear the render target
        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::LightSkyBlue));

        // User our brush to draw/fill the ellipse
        // Possible drawing functions: https://docs.microsoft.com/en-us/windows/win32/api/d2d1/nn-d2d1-id2d1rendertarget
        // Use the ellipse object that is part of MainWindow (and is adjusted accordingly in other parts of the code)
        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::AntiqueWhite));
        pRenderTarget->FillEllipse(ellipse, pBrush);
        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
        pRenderTarget->DrawEllipse(ellipse, pBrush, 10);

        // Get the time of day
        SYSTEMTIME timeOfDay;
        GetLocalTime(&timeOfDay);

        // Calculate angles
        const float hourAngle = 360.0f * (timeOfDay.wHour / 12.0f) 
            + (360.0f / 12.0f) * (timeOfDay.wMinute / 60.0f);
        const float minuteAngle = 360.0f * (timeOfDay.wMinute / 60.0f);
        const float secondAngle = 360.0f * (timeOfDay.wSecond / 60.0f);

        // Draw clock hands
        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
        DrawClockHand(0.55f, hourAngle, 6);
        DrawClockHand(0.7f, minuteAngle, 4);
        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::OrangeRed));
        DrawClockHand(0.9f, secondAngle, 2);

        // Restore the identity transformation.
        pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        // Render the counter text
        std::cout << "OnPaint() invoked at: "
            << std::to_string(timeOfDay.wHour) << ":"
            << std::to_string(timeOfDay.wMinute) << ":"
            << std::to_string(timeOfDay.wSecond) << ":"
            << std::to_string(timeOfDay.wMilliseconds)
            << ". Counter is: " << std::to_string(timeElapsed)
            << std::endl;
        D2D1_SIZE_F size = pRenderTarget->GetSize();
        std::wstring counterText = std::to_wstring(timeElapsed * 10);
        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
        pRenderTarget->DrawText(
            counterText.c_str(),
            counterText.length(),
            pTextFormat,
            D2D1::RectF(0, 0, size.width, size.height),
            pBrush);

        // End D2Direct drawing
        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) {
            // If there was an error (we lost the graphics device or something), we need to re-create the
            // render target and device-dependent resources
            DiscardGraphicsResources();
        }

        // Finish painting
        EndPaint(m_hwnd, &ps);

        // Trigger another repainting immediately
        // Comment this out to use the timer instead
        // RedrawWindow(m_hwnd, NULL, NULL, RDW_INTERNALPAINT);
    }
}

void MainWindow::Resize() {
    // Only need to do anything if we have a valid render target
    if (pRenderTarget != NULL) {
        // Get the new size of the client area (in physical pixels)
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        // Now resize the render target to the new window size (physical pixels)
        pRenderTarget->Resize(size);

        // Also recalculate any shapes we've drawn as part of our stuff
        CalculateLayout();

        // Mark the entire region as part of the update region (forces a WM_PAINT)
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}

void MainWindow::CalculateLayout() {
    // Only need to do anything if we have a valid render target
    if (pRenderTarget != NULL) {
        // Get the size of the render target (in DIPs)
        D2D1_SIZE_F size = pRenderTarget->GetSize();

        // Compute the placement and radius of our ellipse (in middle, circle that fits)
        const float x = size.width / 2;
        const float y = size.height / 2;
        const float radius = min(x, y) - 5;

        // Create a new ellipse for the new size
        ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);
    }
}

void MainWindow::DrawClockHand(float handLength, float angle, float strokeWidth) {
    // Set the rotation matrix of our render target to the appropriate angle rotation matrix
    pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(angle, ellipse.point));

    // Calculate the endpoint of the hand (other end is the centre)
    D2D1_POINT_2F endPoint = D2D1::Point2F(ellipse.point.x, ellipse.point.y - ellipse.radiusY * handLength);

    // Draw the line, asume color chosen before
    pRenderTarget->DrawLine(ellipse.point, endPoint, pBrush, strokeWidth);
}