#pragma once

#include "util.h"

inline Memory* mem;

extern int textures_used;
extern LPDIRECT3DDEVICE9 d3dDevice;

//LATER probably don't need it externally
inline const int filepath_buf_maxlen = 200;

extern HWND g_hwnd;

struct Display {
	int display_w;
	int display_h;
	bool need_redraw_not;
	bool possible_to_draw_not;
};

inline Display g_display;

inline int g_window_w;
inline int g_window_h;

inline float g_window_w_f;
inline float g_window_h_f;

inline int g_window_w_char;
inline int g_window_h_char;



inline constexpr int multithreading = 0;

inline CRITICAL_SECTION render_load_mutex;
inline CRITICAL_SECTION warn_mutex;

inline CRITICAL_SECTION render_load_mutex_short;



struct CUSTOMVERTEX
{
	FLOAT x, y, z, rhw;
	FLOAT tu, tv;
};

// Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW | D3DFVF_TEX1)



inline const int text_warn_max = 5;
inline char* text_warn[text_warn_max] = { 0 };
inline constexpr int text_warn_str_max = 50;

inline DWORD warn_tickcount = 0;

void fill_list(float*& p, float where_l, float where_r, float where_t, float where_b, float what_l, float what_r, float what_t, float what_b);

void warn(const char* string);