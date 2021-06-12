/* ===========================================================
   #File: _d3d9_teapot.cpp #
   #Date: 13 June 2021 #
   #Revision: 1.0 #
   #Creator: Omid Miresmaeili #
   #Description: Working with Direct3D 9.0 meshes,
   # FX framework, triangle grid
   #Notice: (C) Copyright 2021 by Omid. All Rights Reserved. #
   =========================================================== */

#include "Common.h"

#include "DirectInput.h"
#include "Vertex.h"

#include <DearImGui/imgui.h>
#include <DearImGui/imgui_impl_dx9.h>
#include <DearImGui/imgui_impl_win32.h>

#define ENABLE_IMGUI

typedef struct {
    IDirect3DDevice9 *          device;

    D3DDEVTYPE                  device_type;
    DWORD                       requested_vertex_processing;

    // window stuff
    HINSTANCE                   instance;
    HWND                        wnd;
    IDirect3D9 *                d3d9_object;

    D3DPRESENT_PARAMETERS       present_params;

    DWORD                       ngridverts;
    DWORD                       ngridtriangles;

    ID3DXMesh *                 cylinder_mesh;
    ID3DXMesh *                 sphere_mesh;
    ID3DXMesh *                 teapot_mesh;

    IDirect3DVertexBuffer9 *    vb;
    IDirect3DIndexBuffer9 *     ib;
    ID3DXEffect *               fx;
    D3DXHANDLE                  htech;
    D3DXHANDLE                  hwvp;

    float                       camera_rotation_y;
    float                       camera_radius;
    float                       camera_height;

    D3DXMATRIX                  view;
    D3DXMATRIX                  proj;

    bool                        paused;
    bool                        initialized;
} D3D9RenderContext;

D3D9RenderContext * g_render_ctx = nullptr;
DirectInput * g_dinput = nullptr;
VertexPos g_vertex_pos = {};

// Helper functions.

static void
create_fx (D3D9RenderContext * render_ctx) {
    // Create the FX from a .fx file.
    ID3DXBuffer* errors = 0;
    D3DXCreateEffectFromFileA(
        render_ctx->device, ("transform.fx"),
        0, 0, D3DXSHADER_DEBUG, 0, &render_ctx->fx, &errors
    );
    if (errors)
        MessageBoxA(0, (char *)errors->GetBufferPointer(), 0, 0);

    // Obtain handles.
    render_ctx->htech = render_ctx->fx->GetTechniqueByName("transform_tech");
    render_ctx->hwvp  = render_ctx->fx->GetParameterByName(0, "g_wvp");
}

static void
create_view_mat(D3D9RenderContext * render_ctx) {
    float x = render_ctx->camera_radius * cosf(render_ctx->camera_rotation_y);
    float z = render_ctx->camera_radius * sinf(render_ctx->camera_rotation_y);
    D3DXVECTOR3 pos(x, render_ctx->camera_height, z);
    D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
    D3DXMatrixLookAtLH(&render_ctx->view, &pos, &target, &up);
}
static void
create_proj_mat (D3D9RenderContext * render_ctx) {
    float w = (float)render_ctx->present_params.BackBufferWidth;
    float h = (float)render_ctx->present_params.BackBufferHeight;
    D3DXMatrixPerspectiveFovLH(&render_ctx->proj, D3DX_PI * 0.25f, w / h, 1.0f, 5000.0f);
}
static bool
check_device_caps () {
    // nothing to check.
    return true;
}
static void
d3d9_lost_device (D3D9RenderContext * render_ctx) {
    //
}
static void
d3d9_reset_device (D3D9RenderContext * render_ctx) {
    ImGui_ImplDX9_InvalidateDeviceObjects();
    render_ctx->device->Reset(&render_ctx->present_params);
    ImGui_ImplDX9_CreateDeviceObjects();

    render_ctx->fx->OnResetDevice();

    // The aspect ratio depends on the backbuffer dimensions, which can 
    // possibly change after a reset.  So rebuild the projection matrix.
    create_proj_mat(render_ctx);
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
    d3d9_reset_device(render_ctx);
}
static void
update_scene (D3D9RenderContext * render_ctx, float dt) {
    // Get snapshot of input devices.
    DirectInput_Poll(g_dinput);

    // Check input.
    if (DirectInput_KeyDown(g_dinput, DIK_W))
        render_ctx->camera_height   += 25.0f * dt;
    if (DirectInput_KeyDown(g_dinput, DIK_S))
        render_ctx->camera_height   -= 25.0f * dt;

    // Divide by 50 to make mouse less sensitive. 
    render_ctx->camera_rotation_y += DirectInput_MouseDx(g_dinput) / 100.0f;
    render_ctx->camera_radius    += DirectInput_MouseDy(g_dinput) / 25.0f;

    // If we rotate over 360 degrees, just roll back to 0
    if (fabsf(render_ctx->camera_rotation_y) >= 2.0f * D3DX_PI)
        render_ctx->camera_rotation_y = 0.0f;

    // Don't let radius get too small.
    if (render_ctx->camera_radius < 5.0f)
        render_ctx->camera_radius = 5.0f;

    // The camera position/orientation relative to world space can 
    // change every frame based on input, so we need to rebuild the
    // view matrix every frame with the latest changes.
    create_view_mat(render_ctx);
}
static void
draw_teapot (D3D9RenderContext * render_ctx) {
    D3DXMATRIX T;
    D3DXMatrixTranslation(&T, 2.0f, 2.0f, -2.0f);
    D3DXMATRIX view_proj = T * render_ctx->view * render_ctx->proj;
    render_ctx->fx->SetMatrix(render_ctx->hwvp, &view_proj);
    render_ctx->fx->CommitChanges();
    render_ctx->teapot_mesh->DrawSubset(0);
}
static void
draw_scene (D3D9RenderContext * render_ctx) {

    render_ctx->device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0);

    render_ctx->device->BeginScene();

    // setup fx
    render_ctx->fx->SetTechnique(render_ctx->htech);

    // begin pass
    UINT n_passes = 0;
    render_ctx->fx->Begin(&n_passes, 0);
    for (UINT i = 0; i < n_passes; i++) {
        render_ctx->fx->BeginPass(i);

        draw_teapot(render_ctx);

        render_ctx->fx->EndPass();
    }
    render_ctx->fx->End();

#ifdef ENABLE_IMGUI
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
#endif // ENABLE_IMGUI

    render_ctx->device->EndScene();
    render_ctx->device->Present(0, 0, 0, 0);    // present backbuffer
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
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK
wnd_proc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;
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
    ::AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    render_ctx->wnd = CreateWindow(
        _T("D3DWndClassName"),
        _T("D3D9 Demo"),
        WS_OVERLAPPEDWINDOW, 100, 100, R.right, R.bottom,
        0, 0, render_ctx->instance, 0
    );

    ::ShowWindow(render_ctx->wnd, SW_SHOW);
    ::UpdateWindow(render_ctx->wnd);
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

        render_ctx->camera_radius = 10.0f;
        render_ctx->camera_rotation_y = 1.2 * D3DX_PI;
        render_ctx->camera_height = 5.0f;

        render_ctx->initialized = true;

        //d3d9_reset_device(render_ctx);
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

    // -- setup direct-input
    g_dinput = (DirectInput *)::malloc(sizeof(DirectInput));
    DirectInput_Init(
        g_dinput,
        DISCL_NONEXCLUSIVE | DISCL_FOREGROUND,
        DISCL_NONEXCLUSIVE | DISCL_FOREGROUND,
        g_render_ctx->instance,
        g_render_ctx->wnd
    );

    // -- create shapes
    D3DXCreateTeapot(g_render_ctx->device, &g_render_ctx->teapot_mesh, 0);

    create_fx(g_render_ctx);

    d3d9_reset_device(g_render_ctx);

    InitAllVertexDeclarations(g_render_ctx->device, &g_vertex_pos);

    // -- setup dear-imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_render_ctx->wnd);
    ImGui_ImplDX9_Init(g_render_ctx->device);

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

                //
                // DearImGui
                // 
                {
                    static float f = 0.0f;
                    static int counter = 0;

                    // -- start the Dear ImGui frame
                    ImGui_ImplDX9_NewFrame();
                    ImGui_ImplWin32_NewFrame();
                    ImGui::NewFrame();
                    ImGui::Begin("D3D9 DearImGui!");                        // Create a window called "Hello, world!" and append into it.

                    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

                    if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                        counter++;
                    ImGui::SameLine();
                    ImGui::Text("counter = %d", counter);

                    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                    ImGui::End();

                    ImGui::EndFrame();
                }

                update_scene(g_render_ctx, dt);
                draw_scene(g_render_ctx);

                // Prepare for next iteration: The current time stamp becomes
                // the previous time stamp for the next iteration.
                prev_time_stamp = curr_time_stamp;

                // -- display results on window's title bar
                TCHAR buf[50];
                _sntprintf_s(buf, 50, 50, _T("D3D9 Teapot Demo:   %s"), _T(""));
                ::SetWindowText(g_render_ctx->wnd, buf);
            }
        }
    }
#pragma endregion
#pragma region Cleanup

    // -- imgui
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    DirectInput_Deinit(g_dinput);

    ::free(g_render_ctx);
#pragma endregion
}


