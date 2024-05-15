#pragma once

void TextInit(int window_w_char, int window_h_char);
void TextInit2();
void RenderText();

void LoadText();
void TextRenderReset();
void Text_WindowSizeChanged(int width, int height);
void Text_OnExit();

bool Text_waiting_for_text_input();
LRESULT Text_MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

inline int char_w = 9;
inline int char_h = 16;