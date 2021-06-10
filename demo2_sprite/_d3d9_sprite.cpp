/* ===========================================================
   #File: _d3d9_sprite.cpp #
   #Date: 10 June 2021 #
   #Revision: 1.0 #
   #Creator: Omid Miresmaeili #
   #Description: Drawing sprites using Direct3D 9.0 #
   #Reference: http://www.d3dcoder.net/d3d9c.htm
   #Notice: (C) Copyright 2021 by Omid. All Rights Reserved. #
   =========================================================== */

#include "Common.h"

#include "DirectInput.h"
#include "BulletArray.h"

#include <DearImGui/imgui.h>
#include <DearImGui/imgui_impl_dx9.h>
#include <DearImGui/imgui_impl_win32.h>

//#define ENABLE_IMGUI

typedef struct {
    IDirect3DDevice9 *      device;

    D3DDEVTYPE              device_type;
    DWORD                   requested_vertex_processing;

    // window stuff
    HINSTANCE               instance;
    HWND                    wnd;
    IDirect3D9 *            d3d9_object;

    D3DPRESENT_PARAMETERS   present_params;

    ID3DXSprite *           sprite;

    // background
    IDirect3DTexture9 *     bg_tex;
    D3DXVECTOR3             bg_center;

    // bullet data
    IDirect3DTexture9 *     bullet_tex;
    D3DXVECTOR3             bullet_center;
    float                   bullet_speed;

    // ship data
    IDirect3DTexture9 *     ship_tex;
    D3DXVECTOR3             ship_center;
    D3DXVECTOR3             ship_pos;
    float                   ship_rotation;
    float                   ship_speed;

    float                   ship_max_speed;
    float                   ship_accel;
    float                   ship_drag;

    bool                    paused;
    bool                    initialized;
} D3D9RenderContext;

D3D9RenderContext * g_render_ctx = nullptr;
DirectInput * g_dinput = nullptr;
BulletArray * g_bullets = nullptr;

// Helper functions.
static void
update_ship (D3D9RenderContext * render_ctx, float dt) {
    // Check input.
    if (DirectInput_KeyDown(g_dinput, DIK_A))   render_ctx->ship_rotation += 4.0f * dt;
    if (DirectInput_KeyDown(g_dinput, DIK_D))   render_ctx->ship_rotation -= 4.0f * dt;
    if (DirectInput_KeyDown(g_dinput, DIK_W))   render_ctx->ship_speed    += render_ctx->ship_accel * dt;
    if (DirectInput_KeyDown(g_dinput, DIK_S))   render_ctx->ship_speed    -= render_ctx->ship_accel * dt;

    // Clamp top speed.
    if (render_ctx->ship_speed > render_ctx->ship_max_speed)    render_ctx->ship_speed =  render_ctx->ship_max_speed;
    if (render_ctx->ship_speed < -render_ctx->ship_max_speed)   render_ctx->ship_speed = -render_ctx->ship_max_speed;

    // Rotate counterclockwise when looking down -z axis (i.e., rotate
    // clockwise when looking down the +z axis.
    D3DXVECTOR3 ship_dir(-sinf(render_ctx->ship_rotation), cosf(render_ctx->ship_rotation), 0.0f);

    // Update position and speed based on time.
    render_ctx->ship_pos   += ship_dir * render_ctx->ship_speed * dt;
    render_ctx->ship_speed -= render_ctx->ship_drag * render_ctx->ship_speed * dt;
}
static void update_bullets (D3D9RenderContext * render_ctx, float dt) {
    // Make static so that its value persists across function calls.
    static float fire_delay = 0.0f;

    // Accumulate time.
    fire_delay += dt;

    // Did the user press the spacebar key and has 0.1 seconds passed?
    // We can only fire one bullet every 0.1 seconds.  If we do not
    // put this delay in, the ship will fire bullets way too fast.
    if (DirectInput_KeyDown(g_dinput, DIK_SPACE) && fire_delay > 0.1f) {
        // Remember the ship is always drawn at the center of the window--
        // the origin.  Therefore, bullets originate from the origin.
        D3DXVECTOR3 pos      = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

        // The bullets rotation should match the ship's rotating at the
        // instant it is fired.  
        float rotation = render_ctx->ship_rotation;

        // Bullet just born.
        float life     = 0.0f;

        // Add the bullet to the list.
        BulletArray_AddItem(g_bullets, pos, rotation, life);

        // A bullet was just fired, so reset the fire delay.
        fire_delay = 0.0f;
    }

    // loop through bullets and update positions
    int i = 0;
    while (i < BulletArray_Count(g_bullets)) {
        BulletArray_UpdateItemPos(g_bullets, i, dt);
        ++i;
    }
}
static void draw_bg (D3D9RenderContext * render_ctx) {
    // Set a texture coordinate scaling transform.  Here we scale the texture 
    // coordinates by 10 in each dimension.  This tiles the texture 
    // ten times over the sprite surface.
    D3DXMATRIX tex_scale;
    D3DXMatrixScaling(&tex_scale, 10.0f, 10.0f, 0.0f);
    render_ctx->device->SetTransform(D3DTS_TEXTURE0, &tex_scale);

    // Position and size the background sprite--remember that 
    // we always draw the ship in the center of the client area 
    // rectangle. To give the illusion that the ship is moving,
    // we translate the background in the opposite direction.
    D3DXMATRIX T, S, ST;
    D3DXMatrixTranslation(&T, -render_ctx->ship_pos.x, -render_ctx->ship_pos.y, -render_ctx->ship_pos.z);
    D3DXMatrixScaling(&S, 20.0f, 20.0f, 0.0f);
    ST = S * T;
    render_ctx->sprite->SetTransform(&ST);

    // Draw the background sprite.
    render_ctx->sprite->Draw(render_ctx->bg_tex, 0, &render_ctx->bg_center, 0, D3DCOLOR_XRGB(255, 255, 255));
    render_ctx->sprite->Flush();

    // Restore defaults texture coordinate scaling transform.
    D3DXMatrixScaling(&tex_scale, 1.0f, -1.0f, 0.0f);
    render_ctx->device->SetTransform(D3DTS_TEXTURE0, &tex_scale);
}
static void draw_ship (D3D9RenderContext * render_ctx) {
    // Turn on the alpha test.
    render_ctx->device->SetRenderState(D3DRS_ALPHATESTENABLE, true);

    // Set ships orientation.
    D3DXMATRIX R;
    D3DXMatrixRotationZ(&R, render_ctx->ship_rotation);
    render_ctx->sprite->SetTransform(&R);

    // Draw the ship.
    render_ctx->sprite->Draw(render_ctx->ship_tex, 0, &render_ctx->ship_center, 0, D3DCOLOR_XRGB(255, 255, 255));
    render_ctx->sprite->Flush();

    // Turn off the alpha test.
    render_ctx->device->SetRenderState(D3DRS_ALPHATESTENABLE, false);
}
static void draw_bullets (D3D9RenderContext * render_ctx) {
    // Turn on alpha blending.
    render_ctx->device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);

    for (int i = 0; i < BulletArray_Count(g_bullets); ++i) {
        D3DXVECTOR3 pos = BulletArray_GetItemPos(g_bullets, i);
        float rotation = BulletArray_GetItemRotation(g_bullets, i);

        // Set its position and orientation.
        D3DXMATRIX T, R, RT;
        D3DXMatrixRotationZ(&R, rotation);
        D3DXMatrixTranslation(&T, pos.x, pos.y, pos.z);
        RT = R * T;
        render_ctx->sprite->SetTransform(&RT);

        // Add it to the batch.
        render_ctx->sprite->Draw(render_ctx->bullet_tex, 0, &render_ctx->bullet_center, 0, D3DCOLOR_XRGB(255, 255, 255));
        ++i;
    }
    // Draw all the bullets at once.
    render_ctx->sprite->Flush();

    // Turn off alpha blending.
    render_ctx->device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
}
static bool
check_device_caps () {
    // nothing to check.
    return true;
}
static void
d3d9_lost_device (D3D9RenderContext * render_ctx) {
    render_ctx->sprite->OnLostDevice();
}
static void
d3d9_reset_device (D3D9RenderContext * render_ctx) {
    ImGui_ImplDX9_InvalidateDeviceObjects();
    render_ctx->device->Reset(&render_ctx->present_params);
    ImGui_ImplDX9_CreateDeviceObjects();

    render_ctx->sprite->OnResetDevice();

    // Sets up the camera 1000 units back looking at the origin.
    D3DXMATRIX V;
    D3DXVECTOR3 pos(0.0f, 0.0f, -1000.0f);
    D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
    D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
    D3DXMatrixLookAtLH(&V, &pos, &target, &up);
    render_ctx->device->SetTransform(D3DTS_VIEW, &V);

    // The following code defines the volume of space the camera sees.
    D3DXMATRIX P;
    RECT R;
    GetClientRect(render_ctx->wnd, &R);
    float width  = (float)R.right;
    float height = (float)R.bottom;
    D3DXMatrixPerspectiveFovLH(&P, D3DX_PI * 0.25f, width / height, 1.0f, 5000.0f);
    render_ctx->device->SetTransform(D3DTS_PROJECTION, &P);

    // This code sets texture filters, which helps to smooth out distortions
    // when you scale a texture.  
    render_ctx->device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    render_ctx->device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    render_ctx->device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

    // This line of code disables Direct3D lighting.
    render_ctx->device->SetRenderState(D3DRS_LIGHTING, false);

    // The following code specifies an alpha test and reference value.
    render_ctx->device->SetRenderState(D3DRS_ALPHAREF, 10);
    render_ctx->device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);

    // The following code is used to setup alpha blending.
    render_ctx->device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    render_ctx->device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    render_ctx->device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    render_ctx->device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    // Indicates that we are using 2D texture coordinates.
    render_ctx->device->SetTextureStageState(
        0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2
    );
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
    DirectInput_Poll(g_dinput);

    update_ship(render_ctx, dt);
    update_bullets(render_ctx, dt);
}
static void
draw_scene (D3D9RenderContext * render_ctx) {
    render_ctx->device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0);

    render_ctx->device->BeginScene();

    render_ctx->sprite->Begin(D3DXSPRITE_OBJECTSPACE | D3DXSPRITE_DONOTMODIFY_RENDERSTATE);
    draw_bg(render_ctx);
    draw_ship(render_ctx);
    draw_bullets(render_ctx);

    render_ctx->sprite->End();

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
        _T("D3D9 Sprite"),
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

        // ships and bullets
        render_ctx->bullet_speed = 2500.0f;
        render_ctx->ship_max_speed = 1500.0f;
        render_ctx->ship_accel = 1000.0f;
        render_ctx->ship_drag = 0.85f;

        D3DXCreateSprite(render_ctx->device, &render_ctx->sprite);

        D3DXCreateTextureFromFile(render_ctx->device, _T("bkgd1.bmp"), &render_ctx->bg_tex);
        D3DXCreateTextureFromFile(render_ctx->device, _T("alienship.bmp"), &render_ctx->ship_tex);
        D3DXCreateTextureFromFile(render_ctx->device, _T("bullet.bmp"), &render_ctx->bullet_tex);

        render_ctx->bg_center = D3DXVECTOR3(256.0f, 256.0f, 0.0f);
        render_ctx->ship_center = D3DXVECTOR3(64.0f, 64.0f, 0.0f);
        render_ctx->bullet_center = D3DXVECTOR3(32.0f, 32.0f, 0.0f);

        render_ctx->ship_pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        render_ctx->ship_speed = 0.0f;
        render_ctx->ship_rotation = 0.0f;

        render_ctx->initialized = true;

        d3d9_reset_device(render_ctx);
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

    // -- setup direct-input
    g_dinput = (DirectInput *)::malloc(sizeof(DirectInput));
    DirectInput_Init(
        g_dinput,
        DISCL_NONEXCLUSIVE | DISCL_FOREGROUND,
        DISCL_NONEXCLUSIVE | DISCL_FOREGROUND,
        g_render_ctx->instance,
        g_render_ctx->wnd
    );

    // -- setup bullet-list
    size_t bullets_size = BulletArray_CalcRequiredSize();
    BYTE * bullets_memory = (BYTE *)::malloc(bullets_size);
    g_bullets = BulletArray_Init(bullets_memory);

    // -- setup dear-imgui
    // Setup Dear ImGui context
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
                _sntprintf_s(buf, 50, 50, _T("D3D9 Sprite Demo:   Bullet Count: %d"), BulletArray_Count(g_bullets));
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


    BulletArray_Deinit(g_bullets);
    ::free(bullets_memory);

    DirectInput_Deinit(g_dinput);

    ::free(g_render_ctx);
#pragma endregion
}


