
#include "pch.h"

#include "main.h"
#include "util.h"
#include "TextEditor.h"
//#include "Gallery.h"

//#include <Windows.h>

#include <vector>
using namespace std;



//TODO: maybe some check on unicode characters. It is possible for the first byte of utf8 character to be followed by the end of the file, but very unlikely.


//VARS

HWND g_hwnd;


int g_background_color = 0x00000000;

int textures_used = 0;

LPDIRECT3D9             g_pD3D = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       d3dDevice = NULL; // Our rendering device




//VARSEND


//FUNCAHEAD

void warn(const char* string);



//FUNC




void warn(const char* string) {
	//g_text_have_warning = true;

	if (multithreading)
		EnterCriticalSection(&warn_mutex);

	warn_tickcount = (GetTickCount() + 4000) | 1;
	//| 1 is to make sure it's never 0.
	//i'm losing a bit of precision, but it doesn't matter and it's not very precise anyway.

	//don't just store it, copy it somewhere
	//for (int i = 0; i < text_warn_max; i++) {
	//	if (text_warn[i]) continue;
	//	text_warn[i] = (char*)string;
	//	return;
	//}

	if (text_warn[0] == 0) {
		text_warn[0] = (char*)malloc(text_warn_str_max);
	}

	strcpy_s(text_warn[0], text_warn_str_max, string);





	//LATER
	//if (i=text_warn_max) THROW();

	if (multithreading)
		LeaveCriticalSection(&warn_mutex);
}


//LATER: delete this, I never call it
VOID Cleanup()
{
	//GalleryCleanup();

	if (d3dDevice != nullptr) {
		d3dDevice->Release();
		d3dDevice = nullptr;
	}

	if (g_pD3D != nullptr) {
		g_pD3D->Release();
		g_pD3D = nullptr;
	}
}


void fill_list(float*& p, float where_l, float where_r, float where_t, float where_b, float what_l, float what_r, float what_t, float what_b) {
	/*lt*/
	*p++ = where_l;
	*p++ = where_t;
	*p++ = 0.5f;
	*p++ = 1.0f;
	*p++ = what_l;
	*p++ = what_t;

	/* rt */
	*p++ = where_r;
	*p++ = where_t;
	*p++ = 0.5f;
	*p++ = 1.0f;
	*p++ = what_r;
	*p++ = what_t;

	/*lb*/
	*p++ = where_l;
	*p++ = where_b;
	*p++ = 0.5f;
	*p++ = 1.0f;
	*p++ = what_l;
	*p++ = what_b;

	/*lb*/
	*p++ = where_l;
	*p++ = where_b;
	*p++ = 0.5f;
	*p++ = 1.0f;
	*p++ = what_l;
	*p++ = what_b;

	/*rt*/
	*p++ = where_r;
	*p++ = where_t;
	*p++ = 0.5f;
	*p++ = 1.0f;
	*p++ = what_r;
	*p++ = what_t;

	/*rb*/
	*p++ = where_r;
	*p++ = where_b;
	*p++ = 0.5f;
	*p++ = 1.0f;
	*p++ = what_r;
	*p++ = what_b;

}


void RenderReset() {

	TextRenderReset();



	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.BackBufferWidth = g_window_w;
	d3dpp.BackBufferHeight = g_window_h;



	HRESULT res = d3dDevice->Reset(&d3dpp);
	if (res != D3D_OK) {
		THROW();

	}

	//text_unicode_get();
}






//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Draws the scene
//-----------------------------------------------------------------------------
VOID Render()
{

	// Clear the backbuffer to a blue (nope, BLACK) color
	//g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB( 0, 0, 0 ), 1.0f, 0 );
	//g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0x0000FFFF, 1.0f, 0);
	d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, g_background_color, 1.0f, 0);

	// Begin the scene
	d3dDevice->BeginScene();

	d3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

	if (multithreading) {
		EnterCriticalSection(&render_load_mutex);
	}

	//GalleryRender();

	RenderText();

	// End the scene
	d3dDevice->EndScene();

	// Present the backbuffer contents to the display
	d3dDevice->Present(NULL, NULL, NULL, NULL);

	if (multithreading) {
		LeaveCriticalSection(&render_load_mutex);
	}
}



LRESULT WINAPI wWinMain_MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{


	int is_repeat;
	UINT32 scancode;

	switch (msg)
	{
	case WM_SETFOCUS:
	{
		g_display.need_redraw_not = 0;
		return 0;
	}
	case WM_SIZE:
	{
		//return 0;

		g_window_w_f = g_display.display_w = g_window_w = (int)(short)LOWORD(lParam);
		g_window_h_f = g_display.display_h = g_window_h = (int)(short)HIWORD(lParam);
		g_display.need_redraw_not = 0;
		if (g_window_w == 0 && g_window_h == 0) {
			g_display.possible_to_draw_not = 1;
			return 0;
		}
		else g_display.possible_to_draw_not = 0;

		//LATER, I do two divisions in a row, and I'm not sure which of those variables I even need at all.
		g_window_w_char = g_display.display_w / char_w;
		g_window_h_char = g_display.display_h / char_h;

		Text_WindowSizeChanged(g_display.display_w, g_display.display_h);

		RenderReset();

		return 0;
	}
	case WM_CLOSE:
		//you may just comment this out
		DestroyWindow(hWnd);
		//PostQuitMessage(0);
		return 0;

	case WM_DESTROY:
	{

		Text_OnExit();

		Cleanup();
		PostQuitMessage(0);
		return 0;
	}

	}

	Text_MsgProc(hWnd, msg, wParam, lParam);

	//if (text_editor_mode) {
	if (true) {}
	else { //if (text_editor_mode == 1)

	}



	return DefWindowProc(hWnd, msg, wParam, lParam);
}



INT WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, INT)
{


	memNotSimple = memNotSimple->Init();

	//auto mems = MemorySimple::New(
	//	0x40000000,
	//	0x100000);

	//mem4 = Memory::New(
	//	0x40000000,
	//	0x10000000);

	//mem = mem4;

	mem = Memory::New(
		0x40000000,
		0x10000000);

	//GalleryInit();

	const bool create_window_or_fullscreen = true;

	// Register the window class
	WNDCLASSEX wc =
	{
		sizeof(WNDCLASSEX), CS_CLASSDC, wWinMain_MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL,
		L"showjpg", NULL
	};
	RegisterClassEx(&wc);



	g_window_w = GetSystemMetrics(SM_CXSCREEN);
	g_window_h = GetSystemMetrics(SM_CYSCREEN);





	if (create_window_or_fullscreen) {

		RECT rc = { 0, 0, 0, 0 };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, false);

		g_display.display_w = g_window_w + rc.left - rc.right;
		g_display.display_h = g_window_h + rc.top - rc.bottom;
	}
	else {
		g_display.display_w = g_window_w;
		g_display.display_h = g_window_h;
	}

	g_window_w_f = (float)g_display.display_w;
	g_window_h_f = (float)g_display.display_h;

	g_window_w_char = g_display.display_w / char_w;
	g_window_h_char = g_display.display_h / char_h;

	TextInit(g_window_w_char, g_window_h_char);



	HWND hWnd;

	if (create_window_or_fullscreen) {
		// Create the application's window
		hWnd = CreateWindow(L"showjpg", L"showjpg",
			WS_OVERLAPPEDWINDOW, 0, 0, g_window_w, g_window_h,
			NULL, NULL, wc.hInstance, NULL);

	}
	else {
		//winapi borderless window fullscreen
		//https://stackoverflow.com/questions/34462445/fullscreen-vs-borderless-window

		hWnd = CreateWindow(L"showjpg", L"showjpg",
			WS_POPUP, 0, 0, g_window_w, g_window_h,
			NULL, NULL, wc.hInstance, NULL);
	}

	g_hwnd = hWnd;

	//const bool need_to_cover_taskbar = true;
	//if (need_to_cover_taskbar) {
	//	int style = GetWindowLong(hWnd, GWL_STYLE);
	//	int ex_style = GetWindowLong(hWnd, GWL_EXSTYLE);
	//	SetWindowLong(hWnd, GWL_STYLE,
	//		style & ~(WS_CAPTION | WS_THICKFRAME));
	//	SetWindowLong(hWnd, GWL_EXSTYLE,
	//		ex_style & ~(WS_EX_DLGMODALFRAME |
	//			WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

	//	SetWindowPos(hWnd, NULL, 0, 0,
	//		g_window_w, g_window_h,
	//		SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	//}


	//really cool effect! "letterbox". but don't need it right now
	//HWND hWnd = CreateWindow( L"showjpg", L"showjpg",
	//                          WS_POPUP, 0, 200, g_window_w, g_window_h-400, //-1 is for the task panel to show upw
	//                          NULL, NULL, wc.hInstance, NULL );

	//HWND hWnd = CreateWindow( L"showjpg", L"showjpg",
 //                             WS_OVERLAPPED, 0, 0, g_window_w, g_window_h,
 //                             NULL, NULL, wc.hInstance, NULL );

	//ShowWindow(hWnd, SW_SHOW);







	if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
		THROW();

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, &d3dDevice)))
	{
		THROW();
	}

	d3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	d3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	d3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	


	LoadText();

	TextInit2();



	// Show the window
	//ShowWindow(hWnd, SW_SHOWDEFAULT);
	ShowWindow(hWnd, SW_MAXIMIZE);
	UpdateWindow(hWnd);

	// Enter the message loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			if (Text_waiting_for_text_input()) {
				TranslateMessage(&msg); //will send WM_CHAR, comment if don't need this
			}
			DispatchMessage(&msg);
		}
		else
		{
			bool busy = false;

			if (!g_display.need_redraw_not && !g_display.possible_to_draw_not) {
				HRESULT res = d3dDevice->TestCooperativeLevel();
				if (res == D3D_OK) {
					g_display.need_redraw_not = true;
					Render();
					busy = true;
				}
				else if (res == D3DERR_DEVICELOST) {
					Sleep(1);
				}
				else if (res == D3DERR_DEVICENOTRESET) {
					//MessageBoxA(0, "D3DERR_DEVICENOTRESET yay", "eh", 0);

					RenderReset();
				}
			}

			//LATER
			//if (!text_editor_enabled) {
			if (false) {
				//GalleryLoadEverything(&busy);
			}
			if (!busy) {
				Sleep(1);
			}
		}
	}


	//UnregisterClass( L"showjpg", wc.hInstance );
	return 0;
}



