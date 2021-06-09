/* ===========================================================
   #File: _d3d9_sprite.cpp #
   #Date: 10 June 2021 #
   #Revision: 1.0 #
   #Creator: Omid Miresmaeili #
   #Description: Drawing sprites using Direct3D 9.0 #
   #Notice: (C) Copyright 2021 by Omid. All Rights Reserved. #
   =========================================================== */

#include "Common.h"

#include "DirectInput.h"

typedef struct {
    IDirect3DDevice9 *      device;

    D3DDEVTYPE              device_type;
    DWORD                   requested_vertex_processing;

    // window stuff
    HINSTANCE               instance;
    HWND                    wnd;
    IDirect3D9 *            d3d9_object;


    ID3DXFont *             font;

    bool                    paused;
    D3DPRESENT_PARAMETERS   present_params;

    bool                    initialized;
} D3D9RenderContext;

D3D9RenderContext * g_render_ctx = nullptr;

DirectInput * g_dinput = nullptr;

struct BulletInfo {
    D3DXVECTOR3 pos;
    float rotation;
    float life;
};
static int max_bullet_count = 1'000'000;
struct BulletList {
    int count;
    BulletInfo * items;
};

static void
update_scene (float dt) {
    // no update for now
}
static void
draw_scene (D3D9RenderContext * render_ctx) {
    render_ctx->device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0);

    RECT format_rect;
    GetClientRect(g_render_ctx->wnd, &format_rect);

    render_ctx->device->BeginScene();

    g_render_ctx->font->DrawText(0, _T("Hello Direct3D 9.0"), -1,
        &format_rect, DT_CENTER | DT_VCENTER,
        //D3DCOLOR_XRGB(rand() % 256, rand() % 256, rand() % 256));
        D3DCOLOR_XRGB(255, 0, 0));

    render_ctx->device->EndScene();
    render_ctx->device->Present(0, 0, 0, 0);
}
static bool
check_device_caps () {
    // nothing to check.
    return true;
}
static void
d3d9_lost_device (D3D9RenderContext * render_ctx) {
    render_ctx->font->OnLostDevice();
}
static void
d3d9_reset_device (D3D9RenderContext * render_ctx) {
    render_ctx->font->OnResetDevice();
}
static bool is_device_lost (D3D9RenderContext * render_ctx) {
    // Get the state of the graphics device.
    HRESULT hr = render_ctx->device->TestCooperativeLevel();

    // If the device is lost and cannot be reset yet then
    // sleep for a bit and we'll try again on the next 
    // message loop cycle.
    if (hr == D3DERR_DEVICELOST) {
        Sleep(20);
        return true;
    } else if (hr == D3DERR_DRIVERINTERNALERROR) {
        // Driver error, exit.
        return true;
    } else if (hr == D3DERR_DEVICENOTRESET) {
        // The device is lost but we can reset and restore it.
        d3d9_lost_device(render_ctx);
        render_ctx->device->Reset(&render_ctx->present_params);
        d3d9_reset_device(render_ctx);
        return false;
    } else
        return false;
}
static void
d3d9_enable_fullscreen_mode (D3D9RenderContext * render_ctx, bool enable) {
    // Switch to fullscreen mode.
    if (enable) {
        // Are we already in fullscreen mode?
        if (0 == render_ctx->present_params.Windowed)
            return;

        int width  = GetSystemMetrics(SM_CXSCREEN);
        int height = GetSystemMetrics(SM_CYSCREEN);

        render_ctx->present_params.BackBufferFormat = D3DFMT_X8R8G8B8;
        render_ctx->present_params.BackBufferWidth  = width;
        render_ctx->present_params.BackBufferHeight = height;
        render_ctx->present_params.Windowed         = false;

        // Change the window style to a more fullscreen friendly style.
        SetWindowLongPtr(render_ctx->wnd, GWL_STYLE, WS_POPUP);

        // If we call SetWindowLongPtr, MSDN states that we need to call
        // SetWindowPos for the change to take effect.  In addition, we 
        // need to call this function anyway to update the window dimensions.
        SetWindowPos(render_ctx->wnd, HWND_TOP, 0, 0, width, height, SWP_NOZORDER | SWP_SHOWWINDOW);
    } else { // Switch to windowed mode.
        // Are we already in windowed mode?
        if (render_ctx->present_params.Windowed)
            return;

        RECT R = {0, 0, 800, 600};
        AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
        render_ctx->present_params.BackBufferFormat = D3DFMT_UNKNOWN;
        render_ctx->present_params.BackBufferWidth  = 800;
        render_ctx->present_params.BackBufferHeight = 600;
        render_ctx->present_params.Windowed         = true;

        // Change the window style to a more windowed friendly style.
        SetWindowLongPtr(render_ctx->wnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

        // If we call SetWindowLongPtr, MSDN states that we need to call
        // SetWindowPos for the change to take effect.  In addition, we 
        // need to call this function anyway to update the window dimensions.
        SetWindowPos(render_ctx->wnd, HWND_TOP, 100, 100, R.right, R.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);
    }

    // Reset the device with the changes.
    d3d9_lost_device(render_ctx);
    (render_ctx->device->Reset(&render_ctx->present_params));
    d3d9_reset_device(render_ctx);
}
LRESULT
msg_proc (D3D9RenderContext * render_ctx, UINT msg, WPARAM wparam, LPARAM lparam) {
    // Is the application in a minimized or maximized state?
    static bool min_maxed = false;

    RECT client_rect = {0, 0, 0, 0};
    switch (msg) {

    // WM_ACTIVE is sent when the window is activated or deactivated.
    // We pause the game when the main window is deactivated and 
    // unpause it when it becomes active.
    case WM_ACTIVATE:
        if (LOWORD(wparam) == WA_INACTIVE)
            render_ctx->paused = true;
        else
            render_ctx->paused = false;
        return 0;


    // WM_SIZE is sent when the user resizes the window.  
    case WM_SIZE:
        if (render_ctx->device) {

            render_ctx->present_params.BackBufferWidth  = LOWORD(lparam);
            render_ctx->present_params.BackBufferHeight = HIWORD(lparam);

            if (wparam == SIZE_MINIMIZED) {
                render_ctx->paused = true;
                min_maxed = true;
            } else if (wparam == SIZE_MAXIMIZED) {
                render_ctx->paused = false;
                min_maxed = true;
                d3d9_lost_device(render_ctx);
                (render_ctx->device->Reset(&render_ctx->present_params));
                d3d9_reset_device(render_ctx);
            }
            // Restored is any resize that is not a minimize or maximize.
            // For example, restoring the window to its default size
            // after a minimize or maximize, or from dragging the resize
            // bars.
            else if (wparam == SIZE_RESTORED) {
                render_ctx->paused = false;

                // Are we restoring from a mimimized or maximized state, 
                // and are in windowed mode?  Do not execute this code if 
                // we are restoring to full screen mode.
                if (min_maxed && render_ctx->present_params.Windowed) {
                    d3d9_lost_device(render_ctx);
                    (render_ctx->device->Reset(&render_ctx->present_params));
                    d3d9_reset_device(render_ctx);
                } else {
                    // No, which implies the user is resizing by dragging
                    // the resize bars.  However, we do not reset the device
                    // here because as the user continuously drags the resize
                    // bars, a stream of WM_SIZE messages is sent to the window,
                    // and it would be pointless (and slow) to reset for each
                    // WM_SIZE message received from dragging the resize bars.
                    // So instead, we reset after the user is done resizing the
                    // window and releases the resize bars, which sends a
                    // WM_EXITSIZEMOVE message.
                }
                min_maxed = false;
            }
        }
        return 0;


    // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
    // Here we reset everything based on the new window dimensions.
    case WM_EXITSIZEMOVE:
        GetClientRect(render_ctx->wnd, &client_rect);
        render_ctx->present_params.BackBufferWidth  = client_rect.right;
        render_ctx->present_params.BackBufferHeight = client_rect.bottom;
        d3d9_lost_device(render_ctx);
        (render_ctx->device->Reset(&render_ctx->present_params));
        d3d9_reset_device(render_ctx);

        return 0;

    // WM_CLOSE is sent when the user presses the 'X' button in the
    // caption bar menu.
    case WM_CLOSE:
        DestroyWindow(render_ctx->wnd);
        return 0;

    // WM_DESTROY is sent when the window is being destroyed.
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
            d3d9_enable_fullscreen_mode(render_ctx, false);
        else if (wparam == 'F')
            d3d9_enable_fullscreen_mode(render_ctx, true);
        return 0;
    }
    return DefWindowProc(render_ctx->wnd, msg, wparam, lparam);
}

LRESULT CALLBACK
wnd_proc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    // Don't start processing messages until the application has been created.
    if (g_render_ctx->initialized)
        return msg_proc(g_render_ctx, msg, wparam, lparam);
    else
        return DefWindowProc(hwnd, msg, wparam, lparam);
}
static void
init_window (D3D9RenderContext * render_ctx) {
    WNDCLASS wc;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = wnd_proc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = render_ctx->instance;
    wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName  = 0;
    wc.lpszClassName = _T("D3DWndClassName");
    RegisterClass(&wc);

    if (RegisterClass(&wc)) {
        int x = 10;
    }

    // Default to a window with a client area rectangle of 800x600.
    RECT R = {0, 0, 800, 600};
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    render_ctx->wnd = CreateWindow(
        _T("D3DWndClassName"),
        _T("D3D9 Sprite"),
        WS_OVERLAPPEDWINDOW, 100, 100, R.right, R.bottom,
        0, 0, render_ctx->instance, 0
    );

    ShowWindow(render_ctx->wnd, SW_SHOW);
    UpdateWindow(render_ctx->wnd);
}
static void
init_d3d9 (D3D9RenderContext * render_ctx) {
    // Step 1: Create the IDirect3D9 object.
    render_ctx->d3d9_object = Direct3DCreate9(D3D_SDK_VERSION);

    // Step 2: Verify hardware support for specified formats in windowed and full screen modes.
    D3DDISPLAYMODE mode;
    render_ctx->d3d9_object->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode);
    render_ctx->d3d9_object->CheckDeviceType(D3DADAPTER_DEFAULT, render_ctx->device_type, mode.Format, mode.Format, true);
    render_ctx->d3d9_object->CheckDeviceType(D3DADAPTER_DEFAULT, render_ctx->device_type, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8, false);

    // Step 3: Check for requested vertex processing and pure device.
    D3DCAPS9 caps;
    render_ctx->d3d9_object->GetDeviceCaps(D3DADAPTER_DEFAULT, render_ctx->device_type, &caps);

    DWORD device_behavior_flag = 0;
    if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
        device_behavior_flag |= render_ctx->requested_vertex_processing;
    else
        device_behavior_flag |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;

    // If pure device and HW T&L supported
    if (caps.DevCaps & D3DDEVCAPS_PUREDEVICE &&
        device_behavior_flag & D3DCREATE_HARDWARE_VERTEXPROCESSING)
        device_behavior_flag |= D3DCREATE_PUREDEVICE;

    // Step 4: Fill out the D3DPRESENT_PARAMETERS structure.
    render_ctx->present_params.BackBufferWidth            = 0;
    render_ctx->present_params.BackBufferHeight           = 0;
    render_ctx->present_params.BackBufferFormat           = D3DFMT_UNKNOWN;
    render_ctx->present_params.BackBufferCount            = 1;
    render_ctx->present_params.MultiSampleType            = D3DMULTISAMPLE_NONE;
    render_ctx->present_params.MultiSampleQuality         = 0;
    render_ctx->present_params.SwapEffect                 = D3DSWAPEFFECT_DISCARD;
    render_ctx->present_params.hDeviceWindow              = render_ctx->wnd;
    render_ctx->present_params.Windowed                   = true;
    render_ctx->present_params.EnableAutoDepthStencil     = true;
    render_ctx->present_params.AutoDepthStencilFormat     = D3DFMT_D24S8;
    render_ctx->present_params.Flags                      = 0;
    render_ctx->present_params.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    render_ctx->present_params.PresentationInterval       = D3DPRESENT_INTERVAL_IMMEDIATE;

    // Step 5: Create the device.
    render_ctx->d3d9_object->CreateDevice(
        D3DADAPTER_DEFAULT,             // primary adapter
        render_ctx->device_type,        // device type
        render_ctx->wnd,                // window associated with device
        device_behavior_flag,           // vertex processing
        &render_ctx->present_params,    // present parameters
        &render_ctx->device
    );
}
static void
init_render_ctx (
    D3D9RenderContext * render_ctx,
    HINSTANCE inst, D3DDEVTYPE dev_type, DWORD req_vp
) {
    if (render_ctx) {
        memset(render_ctx, 0, sizeof(*render_ctx));
        render_ctx->device_type = dev_type;
        render_ctx->requested_vertex_processing = req_vp;

        render_ctx->instance = inst;
        render_ctx->wnd = 0;
        render_ctx->d3d9_object = NULL;
        render_ctx->paused = false;

        memset(&render_ctx->present_params, 0, sizeof(render_ctx->present_params));

        init_window(render_ctx);
        init_d3d9(render_ctx);

        render_ctx->initialized = true;
    }
}
int WINAPI
WinMain (
    _In_ HINSTANCE instance,
    _In_opt_ HINSTANCE prev,
    _In_ LPSTR cmdline,
    _In_ int showcmd) {

#pragma region Initialize

    g_render_ctx = (D3D9RenderContext *)::malloc(sizeof(D3D9RenderContext));
    init_render_ctx(
        g_render_ctx,
        instance,
        D3DDEVTYPE_HAL,
        D3DCREATE_HARDWARE_VERTEXPROCESSING
    );

    if (false == check_device_caps())
        MessageBox(0, _T("capability check failed"), 0, 0);

    srand(time_t(0));
    D3DXFONT_DESC font_desc;
    font_desc.Height          = 80;
    font_desc.Width           = 40;
    font_desc.Weight          = FW_BOLD;
    font_desc.MipLevels       = 0;
    font_desc.Italic          = true;
    font_desc.CharSet         = DEFAULT_CHARSET;
    font_desc.OutputPrecision = OUT_DEFAULT_PRECIS;
    font_desc.Quality         = DEFAULT_QUALITY;
    font_desc.PitchAndFamily  = DEFAULT_PITCH | FF_DONTCARE;
    _tcscpy_s(font_desc.FaceName, _T("Times New Roman"));

    D3DXCreateFontIndirect(g_render_ctx->device, &font_desc, &g_render_ctx->font);

    g_dinput = (DirectInput *)::malloc(sizeof(DirectInput));
    DirectInput_Init(g_dinput);

#pragma endregion
#pragma region Main Loop
    MSG  msg;
    msg.message = WM_NULL;
    __int64 cnts_per_sec = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&cnts_per_sec);
    float secs_per_cnt = 1.0f / (float)cnts_per_sec;
    __int64 prev_time_stamp = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&prev_time_stamp);
    while (msg.message != WM_QUIT) {
        // If there are Window messages then process them.
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Otherwise, do animation/game stuff.
        else {
            // If the application is paused then free some CPU 
            // cycles to other applications and then continue on
            // to the next frame.
            if (g_render_ctx->paused) {
                Sleep(20);
                continue;
            }
            if (false == is_device_lost(g_render_ctx)) {
                __int64 curr_time_stamp = 0;
                QueryPerformanceCounter((LARGE_INTEGER*)&curr_time_stamp);
                float dt = (curr_time_stamp - prev_time_stamp) * secs_per_cnt;

                update_scene(dt);
                draw_scene(g_render_ctx);

                // Prepare for next iteration: The current time stamp becomes
                // the previous time stamp for the next iteration.
                prev_time_stamp = curr_time_stamp;
            }
        }
    }
#pragma endregion
#pragma region Cleanup

    DirectInput_Deinit(g_dinput);

    ::free(g_render_ctx);
#pragma endregion
}


