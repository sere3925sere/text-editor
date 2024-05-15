#include "pch.h"

#include "TextEditor.h"
#include "util.h"
#include "main.h"



const bool text_editor_enabled = 1; //this is just for quick disable

char filepath_buf[filepath_buf_maxlen];
char filepath_buf2[filepath_buf_maxlen];


struct TextFileView {

};


struct undo_node {
	struct undo_node* next;
	struct undo_node* prev;
	struct undo_node* split;
	//int buffer_id;
	bool is_split; //only useful when going back
	bool split_dont_follow; //only useful when going forward
	//will merge those two into ->split and ->prev probably
	bool add_or_del; //add is 0, del is 1
	bool direction; //the least rare is 1 //will merely change the position of cursor after undo
	bool is_selection;
	int offset;
	int length;
};



void* undo_root; //probably don't need this, but, why not
struct undo_node* undo_last; //the last one

//struct undo_node* undo_current;

bool undo_committed_not; //if false, when use undo_current during rendering. If true, when it's already written in the buffer.




struct navigation_history_node {
	int buffer_id;
	int cursor;
	int screen_start;
};

const int navigation_history_len = 40;

struct navigation_history {
	int used;
	struct navigation_history* prev;
	//don't need next

	struct navigation_history_node nodes[navigation_history_len];
};


struct text_buffer {
	char path[filepath_buf_maxlen];

	int buffer_id;

	struct text_buffer* text_next;

	FILE* textfile;
	int textfilesize;
	int textfilebufsize;
	char* textfilebuf;

	int screen_start_backup;
	int cursor_backup;

	struct undo_node* undo_current;
};

//LATER: redo it into enum
const int text_editor_mode_navigation = 0;
const int text_editor_mode_edit = 1;
const int text_editor_mode_select = 2;

struct text_view {

	struct text_buffer* text_pointer;

	struct text_view* view_next;

	int screen_start = 0;
	int cursor = 0;
	int selection;

	int text_editor_mode = text_editor_mode_navigation;
	//0 is navigation
	//1 is edit
	//2 is select

	unsigned short xpos, ypos, xsize, ysize;

};

struct text_buffer* text;
struct text_view* g_view;
int text_buffer_id = 0;


bool search_mode = false;

const int search_string_max = 70;
int search_string_used = 0;
char search_string[search_string_max];

bool search_string_selection_active = 0;
int search_string_selection_start = 0;
int search_string_selection_end = 0;

const float char_w_f = 9.0f;
const float char_h_f = 16.0f;


struct UnicodeCoord {
	unsigned short x;
	unsigned short y;
	unsigned char w;
	unsigned char h;
};

List<wchar_t>* unicode_chars = nullptr;
List<UnicodeCoord>* unicodeCoord = nullptr;


const int buffers_max = 0x10;

struct TextureAndBuffer {
	LPDIRECT3DTEXTURE9      texture;
	LPDIRECT3DVERTEXBUFFER9 buffers[buffers_max];
};

const uint32_t vertex_buffer_capacity = 512;
const uint32_t vertex_buffer_capacity_mul = 6;


TextureAndBuffer text_textureandbuffer = { 0 };

TextureAndBuffer unicode_textureandbuffer = { 0 };

//LPDIRECT3DTEXTURE9      unicode_texture = { 0 };
const unsigned int unicode_texture_size = 1024;
const unsigned int unicode_texture_fontsize = 20;

//VARSEND

//FUNCAHEAD

void text_down_where(int whereshouldigo);
void text_up_where(int whereshouldigo);
void text_scroll_down_where(int whereshouldigo);
void text_scroll_up_where(int whereshouldigo);

void text_scroll_sanitize();
void text_right();

void text_copy(HWND);

void text_unicode_get();
void textfile_save();

//FUNCAHEADEND

//FUNC

void TextInit(int width, int height) {
	{
		text = (struct text_buffer*)malloc(sizeof(struct text_buffer));
		ZeroMemory((void*)text, sizeof(struct text_buffer));
		text->text_next = text;



		g_view = (struct text_view*)malloc(sizeof(struct text_view));
		ZeroMemory((void*)g_view, sizeof(struct text_view));

		g_view->xpos = 0;
		g_view->ypos = 0;
		Text_WindowSizeChanged(width, height);

		text_buffer_id = 1;
		text->buffer_id = 1;
		text->text_next = text;

		g_view->text_pointer = text;
		g_view->view_next = g_view;


		//TEXTEDITOR1
		strcpy(text->path, "c:/shared/text/RANDOM.txt");
		text->textfile = fopen(text->path, "rb");

		fseek(text->textfile, 0L, SEEK_END);

		text->textfilebufsize = text->textfilesize = ftell(text->textfile);
		text->textfilebufsize = next_power_of_2(text->textfilebufsize) * 2;
		if (text->textfilebufsize < 0x1000) text->textfilebufsize = 0x1000;

		fseek(text->textfile, 0L, SEEK_SET);

		text->textfilebuf = (char*)malloc(text->textfilebufsize);

		fread((void*)text->textfilebuf, 1, text->textfilesize, text->textfile);

		fclose(text->textfile);

	}

	{
		undo_root = memget();
		//memget_guard("undo_current = undo_root;");

		{
			int* prev = (int*)undo_root;
			*prev = 0;
			int* next = (int*)undo_root + 1;
			*next = 0;
			struct undo_node* node = (struct undo_node*)((int*)undo_root + 2);
			ZeroMemory(node, sizeof(*node));

			text->undo_current = node;
			undo_last = node;

			////comment it out. I need one empty element of the list, to serve as a root
			////this will make code a bit simplier, don't complain
			//const char thing[] = "TypinG_TypinG";
			//node->offset = 1;
			//node->length = sizeof(thing)-1;  //because zero terminated
			//memcpy((void*)(node + 1), thing, sizeof(thing)-1);
			//undo_committed = false;
		}

		//text->undo_current->buffer_id = 1;
	}
}

void TextInit2() {
	text_unicode_get();
}

void TextRenderReset() {
	for (int i = 0; i < buffers_max; i++) {
		if (text_textureandbuffer.buffers[i]) {
			text_textureandbuffer.buffers[i]->Release();
			text_textureandbuffer.buffers[i] = 0;
		}
	}

	for (int i = 0; i < buffers_max; i++) {
		if (unicode_textureandbuffer.buffers[i]) {
			unicode_textureandbuffer.buffers[i]->Release();
			unicode_textureandbuffer.buffers[i] = 0;
		}
	}
}

void Text_WindowSizeChanged(int width, int height) {
	g_view->xsize = width / char_w;
	g_view->ysize = height / char_h;
}

void Text_OnExit() {
	//save all files on quitting
	{
		struct text_buffer* text_start = text;
		do {
			textfile_save();
			text = text->text_next;
		} while (text != text_start);
	}
}

bool Text_waiting_for_text_input() {
	return g_view->text_editor_mode == text_editor_mode_edit || search_mode;
}

#include "texttexturesource.h"

void LoadText() {

	textures_used++;
	d3dDevice->CreateTexture(512, 512, 1, 0,
		D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &(text_textureandbuffer.texture), 0);

	LPDIRECT3DSURFACE9 pSurface;
	text_textureandbuffer.texture->GetSurfaceLevel(0, &pSurface);

	D3DLOCKED_RECT lockedRect;
	pSurface->LockRect(&lockedRect, 0, 0);

	{
		u4* incur = texttexturesource;
		u4* oucur = (u4*)lockedRect.pBits;
		u4 inbits = 0;
		u4 inval = 0;
		for (u4 y = 0; y < 8 * 16; y++) {
			for (u4 x = 0; x < 32 * 9; x++) {
				if (inbits == 0) {
					inval = *incur++;
					inbits = 32;
				}
				if (inval & 0x80000000) {
					*oucur++ = 0xffffffff;
				}
				else {
					*oucur++ = 0x00000000;
				}
				inval = inval << 1;
				inbits--;
			}
			oucur += 512 - 32 * 9;
			//for (int x = 32*9; x < 512; x++) {
			//	*oucur++ = 0x6f6faf6f;
			//}
		}

		incur = texttexturesource;
		for (u4 y = 8 * 16; y < 16 * 16; y++) {
			for (u4 x = 0; x < 32 * 9; x++) {
				if (inbits == 0) {
					inval = *incur++;
					inbits = 32;
				}
				if (inval & 0x80000000) {
					*oucur++ = 0x00000000;
				}
				else {
					*oucur++ = 0xffffffff;
				}
				inval = inval << 1;
				inbits--;
			}
			oucur += 512 - 32 * 9;
			//for (int x = 32*9; x < 512; x++) {
			//	*oucur++ = 0x6faf6f6f;
			//}
		}

		//for (int y = 16*16; y < 512; y++) {
		//	for (int x = 0; x < 512; x++) {
		//		*oucur++ = 0x6f6f6fff;
		//	}
		//}
	}

	pSurface->UnlockRect();
	pSurface->Release();

	//fclose(infile);
}



void text_unicode_to_texture() {
	if (paranoid && unicode_chars == nullptr) THROW();
	u4 count = unicode_chars->Count();
	u4 count_coord = unicodeCoord->Count();
	if (count == count_coord) return;

	auto l_unicode_chars = unicode_chars;


	if (!unicode_textureandbuffer.texture) {
		d3dDevice->CreateTexture(unicode_texture_size, unicode_texture_size, 1, 0,
			D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &(unicode_textureandbuffer.texture), 0);
	}

	LPDIRECT3DSURFACE9 pSurface;
	//g_texture_text->GetSurfaceLevel(0, &pSurface);
	unicode_textureandbuffer.texture->GetSurfaceLevel(0, &pSurface);

	HDC dc;
	pSurface->GetDC(&dc);

	HFONT font = CreateFont(unicode_texture_fontsize, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, TEXT("Courier New"));

	SelectObject(dc, font);
	DeleteObject(font);

	SetTextAlign(dc, TA_UPDATECP);

	auto l_unicode_coords = unicodeCoord;
	while (l_unicode_coords->next.isNotEmpty()) l_unicode_coords = l_unicode_coords->next;
	UnicodeCoord* next;
	l_unicode_coords->reserveLast(l_unicode_coords, next);

	next->x = 0;
	next->y = 0;
	if (count_coord != 0) {
		UnicodeCoord* last = unicodeCoord->Get(count_coord - 1);
		next->x = last->x + last->w * 2;
		next->y = last->y;
	}

	POINT point;
	MoveToEx(dc, next->x, next->y, NULL);

	wchar_t* drawnChar = l_unicode_chars->Get(count_coord);
	wchar_t* drawnChar_end;
	l_unicode_chars->NextInit(drawnChar, drawnChar_end);

	do {

	draw_again:

		//INT out;
		//GetCharWidth32W(dc, next.c, next.c, &out);
		//next.w = out;
		//next.h = unicode_texture_fontsize;

		GetCurrentPositionEx(dc, &point);
		next->x = point.x;
		next->y = point.y;

		SetTextColor(dc, RGB(255, 255, 255));
		SetBkColor(dc, RGB(0, 0, 0));
		ExtTextOutW(dc, 0, 0, NULL, NULL, drawnChar, 1, NULL);

		GetCurrentPositionEx(dc, &point);
		next->w = point.x - next->x + 1;
		next->h = unicode_texture_fontsize;
		MoveToEx(dc, point.x + 1, point.y, NULL);

		SetTextColor(dc, RGB(0, 0, 0));
		SetBkColor(dc, RGB(255, 255, 255));
		ExtTextOutW(dc, 0, 0, NULL, NULL, drawnChar, 1, NULL);

		GetCurrentPositionEx(dc, &point);
		MoveToEx(dc, point.x + 1, point.y, NULL);


		GetCurrentPositionEx(dc, &point);
		if (point.x >= unicode_texture_size) {
			point.x = 0;
			point.y += unicode_texture_fontsize;
			MoveToEx(dc, point.x, point.y, NULL);
			goto draw_again;
		}

		count_coord++;
		if (count_coord >= count) break;
		l_unicode_coords->reserveLast(l_unicode_coords, next);
		l_unicode_chars->Next(l_unicode_chars, drawnChar, drawnChar_end);
		if (paranoid && drawnChar == nullptr) THROW();

	} while (true);





	pSurface->ReleaseDC(dc);
	pSurface->Release();
}



void text_unicode_get_string(char* c, char* cend) {

	if (!unicode_chars) {
		unicode_chars = List<wchar_t>::New(mem);
		unicodeCoord = List<UnicodeCoord>::New(mem);
	}


	while (c <= cend) {
		if ((*c & 0x80) == 0x00) {
			++c;
			continue;
		}
		wchar_t unicode = utf8_to_wchar(c);

		unicode_chars->AddIfNotAlreadyIn(unicode);

	}

	text_unicode_to_texture();
}

void text_unicode_get() {
	char* c = text->textfilebuf;
	char* cend = c + text->textfilesize;
	text_unicode_get_string(c, cend);
}



void textfile_switch_id(int buffer_id) {
	if (text->buffer_id == buffer_id) return;

	text->cursor_backup = g_view->cursor;
	text->screen_start_backup = g_view->screen_start;

	struct text_buffer* l_text_start = text;
	struct text_buffer* l_text = text;
	while (true) {
		if (l_text->buffer_id == buffer_id) goto found;

		l_text = l_text->text_next;
		if (l_text == l_text_start) goto not_found;
	}

found:



	text = l_text;
	g_view->text_pointer = text;

	g_view->cursor = text->cursor_backup;
	g_view->screen_start = text->screen_start_backup;

	return;

not_found:

	THROW();
	return;
}


void undo() {

	undo_committed_not = false;
	if (text->undo_current->prev == 0) {
		//already on oldest
		return;
	}

	if (text->undo_current->add_or_del == false) {
		//addition

		//the juice
		//just shrink it

		int length = text->undo_current->length;

		char* cursor_to = text->textfilebuf + text->undo_current->offset;
		char* cursor_from = text->textfilebuf + text->undo_current->offset + length;
		char* cursor_last = text->textfilebuf + text->textfilesize;
		while (cursor_from < cursor_last) {
			*cursor_to++ = *cursor_from++;
		}

		g_view->cursor = text->undo_current->offset;
		text->textfilesize -= text->undo_current->length;

	}
	else {
		//deletion

		//expand space for your undo text
		//then copy undo text on place

		int length = text->undo_current->length;
		int offset = text->undo_current->offset;
		if (text->undo_current->is_selection) offset += length;

		{
			char* cursor_last = text->textfilebuf + offset - length - 1;
			char* cursor_from = text->textfilebuf + text->textfilesize;
			char* cursor_to = text->textfilebuf + text->textfilesize + length;
			while (cursor_from != cursor_last) {
				*cursor_to-- = *cursor_from--;
			}
		}

		{
			char* cursor_from = (char*)(text->undo_current + 1);
			char* cursor_to = text->textfilebuf + offset - length;
			for (int i = 0; i < length; i++) {
				*cursor_to++ = *cursor_from++;
			}
		}

		g_view->cursor = offset;
		if (text->undo_current->is_selection) g_view->cursor--;
		text->textfilesize += length;

		while ((*(text->textfilebuf + g_view->cursor) & 0xC0) == 0x80) {
			g_view->cursor++;
		}

	}

	text->undo_current = text->undo_current->prev;

	g_display.need_redraw_not = 0;



	text_scroll_sanitize();

}


void undo_redo() {

	if (text->undo_current->next == 0) {
		return;
	}

	//don't really need to bother with undo_committed in redo
	//the only purpose of undo_committed is that you don't move the entire file char by char as you type
	//that commits are happen in packs
	//when you redo, this is exactly what happens

	//yet, what will happen if I try to redo while it's not committed yet?
	//undo_current->next == 0 happens

	//the juice
	if (text->undo_current->split) {
		do {
			text->undo_current = text->undo_current->split;
		} while (text->undo_current->split);
	}
	else {
		if (text->undo_current->next) {
			text->undo_current = text->undo_current->next;
		}
		else {
			return;
		}
	}

	g_display.need_redraw_not = 0;

	g_view->cursor = text->undo_current->offset;

	if (text->undo_current->add_or_del == false) {
		//addition

		int length = text->undo_current->length;

		char* cursor_from;
		char* cursor_to;
		char* cursor_end;

		cursor_from = text->textfilebuf + text->textfilesize;
		cursor_to = text->textfilebuf + text->textfilesize + length;
		cursor_end = text->textfilebuf + g_view->cursor + length - 1;
		while (cursor_to > cursor_end) {
			*cursor_to-- = *cursor_from--;
		}

		//char* c = text->textfilebuf + text->textfilesize;
		//char* cursor_last = text->textfilebuf + g_view->cursor + length - 1;
		//for (; c != cursor_last; c--) {
		//	*c = *(c - length);
		//}

		{
			char* cursor_from = (char*)(text->undo_current + 1);
			char* cursor_to = text->textfilebuf + g_view->cursor; // + cursor - length
			for (int i = 0; i != length; i++) {
				*cursor_to++ = *cursor_from++;
			}
		}

		g_view->cursor = text->undo_current->offset + length;
		text->textfilesize += text->undo_current->length;

	}
	else {
		//deletion

		//just shift characters to the left
		//hm, let's arrange those 3 in the order from smaller to bigger

		if (text->undo_current->is_selection) g_view->cursor += text->undo_current->length;


		char* cursor_to = text->textfilebuf + g_view->cursor - text->undo_current->length; // + cursor - length
		char* cursor_from = text->textfilebuf + g_view->cursor;
		char* cursor_end = text->textfilebuf + text->textfilesize;
		while (cursor_from != cursor_end) {
			*cursor_to++ = *cursor_from++;
		}

		g_view->cursor -= text->undo_current->length;
		text->textfilesize -= text->undo_current->length;
	}

	text_scroll_sanitize();

}


void undo_commit() {
	if (undo_committed_not == true) {
		undo_committed_not = false;
		////different commit depending on if this is an addition or a deletion
		////right now only addition
		//if (undo_current->add_or_del == false) {
		//	//addition
		//	int length = undo_current->length;

		//	{
		//		char* cursor_from = textfilebuf + textfilesize;
		//		char* cursor_to = textfilebuf + textfilesize + length;
		//		char* cursor_last = textfilebuf + cursor + length - 1;
		//		for (; cursor_to != cursor_last; ) {
		//			*cursor_to-- = *cursor_from--;
		//		}
		//	}

		//	{
		//		char* cursor_from = (char*)(undo_current + 1);
		//		char* cursor_to = textfilebuf + cursor; // + cursor - length
		//		for (int i = 0; i != length; i++) {
		//			*cursor_to++ = *cursor_from++;
		//		}
		//	}

		//	//*cursor_offset = wParam;

		//	cursor += length;
		//	textfilesize += length;
		//}
		//else {
		//	//deletion
		//	//just shift characters to the left
		//	//hm, let's arrange those 3 in the order from smaller to bigger
		//	char* cursor_to = textfilebuf + cursor - undo_current->length; // + cursor - length
		//	char* cursor_from = textfilebuf + cursor;
		//	char* cursor_end = textfilebuf + textfilesize;
		//	while (cursor_from != cursor_end) {
		//		*cursor_to++ = *cursor_from++;
		//	}

		//	cursor -= undo_current->length;
		//	textfilesize -= undo_current->length;
		//}

	}
}


void undo_create(int min_length) {

	if (text->undo_current->next == 0) {

		char* end_of_node = (char*)((intptr_t)text->undo_current & ~0xFFF) + 0x1000;
		text->undo_current = (struct undo_node*)((char*)undo_last + sizeof(struct undo_node) + undo_last->length);

		if ((char*)text->undo_current + sizeof(*text->undo_current) + min_length > end_of_node) {
			text->undo_current = (struct undo_node*)memget();

			//this is for cases when min_length > 0x1000
			int length_left = min_length - 0x1000;
			while (length_left > 0) {
				//LATER create a memget that allocates a lot at once.
				memget();
				length_left -= 0x1000;
			}
		}
		ZeroMemory(text->undo_current, sizeof(*text->undo_current));

		undo_last->next = text->undo_current;
		text->undo_current->prev = undo_last;
		undo_last = text->undo_current;


	}
	else {
		//use ->split
		struct undo_node* undo_prev = text->undo_current;

		char* end_of_node = (char*)((intptr_t)text->undo_current & ~0xFFF) + 0x1000;
		text->undo_current = (struct undo_node*)((char*)undo_last + sizeof(struct undo_node) + undo_last->length);
		if ((char*)text->undo_current + sizeof(*text->undo_current) + min_length > end_of_node) {
			text->undo_current = (struct undo_node*)memget();

			//this is for cases when min_length > 0x1000
			int length_left = min_length - 0x1000;
			while (length_left > 0) {
				memget();
				length_left = length_left - 0x1000;
			}
		}
		ZeroMemory(text->undo_current, sizeof(*text->undo_current));

		undo_prev->split = text->undo_current;
		text->undo_current->prev = undo_prev;
		undo_last = text->undo_current;

	}

	//text->undo_current->buffer_id = text->buffer_id;
}

void textfile_save() {
	text->textfile = fopen(text->path, "wb");

	fwrite((void*)text->textfilebuf, 1, text->textfilesize, text->textfile);

	fclose(text->textfile);
}

void textfile_open() {
	undo_commit();

	g_display.need_redraw_not = 0;

	text->cursor_backup = g_view->cursor;
	text->screen_start_backup = g_view->screen_start;

	//find if input is global or not
	bool isglobal = 0;

	if (filepath_buf[0] == '\\' && filepath_buf[1] == '\\') isglobal = true;

	if (!isglobal)
	{
		for (char* cur = filepath_buf; *cur != 0; cur++) {
			if (*cur == ':') {
				isglobal = true;
			}
		}
	}

	if (isglobal) {
		strcpy(filepath_buf2, filepath_buf);
	}
	else {
		strcpy(filepath_buf2, text->path);

		//find last / of source
		char* lastslash = filepath_buf2;
		{
			for (char* cur = filepath_buf2; *cur != 0; cur++) {
				if (*cur == '/' || *cur == '\\') {
					lastslash = cur;
				}
			}
		}

		strcpy(lastslash + 1, filepath_buf);
	}

	//next step, is see if this file is already open
	//if it is, just switch to it

	{
		struct text_buffer* l_text_start = text;
		struct text_buffer* l_text = text;
		while (true) {
			if (strcmp(l_text->path, filepath_buf2) == 0) {
				//FOUND!

				text = l_text;
				g_view->text_pointer = text;

				g_view->cursor = text->cursor_backup;
				g_view->screen_start = text->screen_start_backup;



				return;
			}

			l_text = l_text->text_next;
			if (l_text == l_text_start) break;
		}
	}

	FILE* file = fopen(filepath_buf2, "rb");
	if (!file) return;



	text_buffer* middle = (struct text_buffer*)malloc(sizeof(struct text_buffer));
	ZeroMemory((void*)middle, sizeof(struct text_buffer));

	text_buffer* prev = text;
	text_buffer* next = text->text_next;

	text = middle;
	prev->text_next = middle;
	middle->text_next = next;

	text_buffer_id++;
	text->buffer_id = text_buffer_id;


	g_view->text_pointer = text;

	g_view->screen_start = 0;
	g_view->cursor = 0;



	strcpy(text->path, filepath_buf2);
	text->textfile = file;

	fseek(text->textfile, 0L, SEEK_END);

	text->textfilebufsize = text->textfilesize = ftell(text->textfile);
	text->textfilebufsize = next_power_of_2(text->textfilebufsize) * 2;
	if (text->textfilebufsize < 0x1000) text->textfilebufsize = 0x1000;

	fseek(text->textfile, 0L, SEEK_SET);

	text->textfilebuf = (char*)malloc(text->textfilebufsize);

	fread((void*)text->textfilebuf, 1, text->textfilesize, text->textfile);

	fclose(text->textfile);

	{
		char* end_of_node = (char*)((intptr_t)text->undo_current & ~0xFFF) + 0x1000;
		text->undo_current = (struct undo_node*)((char*)undo_last + sizeof(struct undo_node) + undo_last->length);
		if ((char*)text->undo_current + sizeof(*text->undo_current) > end_of_node) {
			text->undo_current = (struct undo_node*)memget();

		}
		ZeroMemory(text->undo_current, sizeof(*text->undo_current));

		undo_last = text->undo_current;
	}

	text_unicode_get();
}






void text_goto_string_start_skiptabs() {

	char* cur = text->textfilebuf + g_view->cursor;

	if (g_view->cursor == 0 || *(cur - 1) == '\n') {
		//skiptab
		while (*cur == '\t') cur++;
		goto func_end;
	}

	if (cur <= text->textfilebuf) {
	}
	else {
		cur--;
		while (true) {
			if (cur <= text->textfilebuf) break;
			char c = *cur;
			if (c == '\n') {
				cur++;
				break;
			}
			cur--;
		}
	}

func_end:

	g_view->cursor = cur - text->textfilebuf;

	g_display.need_redraw_not = 0;
}


void text_goto_string_start() {

	char* cur = text->textfilebuf + g_view->cursor;

	if (cur <= text->textfilebuf) {
	}
	else {
		cur--;
		while (true) {
			if (cur <= text->textfilebuf) break;
			char c = *cur;
			if (c == '\n') {
				cur++;
				break;
			}
			cur--;
		}
	}

	g_view->cursor = cur - text->textfilebuf;

	g_display.need_redraw_not = 0;
}


void text_goto_string_end() {
	char* cur = text->textfilebuf + g_view->cursor;
	char* cur_end = text->textfilebuf + text->textfilesize;

	while (true) {
		if (cur == cur_end) break;
		if (*cur == '\n') {
			break;
		}
		cur++;
	}

	g_view->cursor = cur - text->textfilebuf;

	g_display.need_redraw_not = 0;
}

void text_insert_newline_and_tabs() {
	text_goto_string_start();
	int number_of_tabs = 0;
	char* cur = text->textfilebuf + g_view->cursor;
	while (*cur == '\t') {
		cur++;
		number_of_tabs++;
	}
	text_goto_string_end();
	//text_insert

	int length = number_of_tabs + 1;


	undo_commit();
	undo_create(length);
	undo_committed_not = true;
	text->undo_current->offset = g_view->cursor;
	text->undo_current->length = length;

	//fill the undo buffer
	char* str = (char*)(text->undo_current + 1);
	char* temp_cur = (char*)(text->undo_current + 1);
	*temp_cur++ = '\n';
	while (number_of_tabs--) *temp_cur++ = '\t';
	//memcpy((char*)(text->undo_current + 1), (char*)str, length);

	//now copy it into the text buffer

	//allocate more space if necessary
	//first free some space
	//next copy the string in freed space

	if (text->textfilesize + length >= text->textfilebufsize) {
		text->textfilebufsize = next_power_of_2(text->textfilesize + length) * 2;
		char* nextbuf = (char*)malloc(text->textfilebufsize);
		memcpy(nextbuf, text->textfilebuf, text->textfilesize);
		free(text->textfilebuf);
		text->textfilebuf = nextbuf;
	}

	{
		char* cursor_end = text->textfilebuf + g_view->cursor;
		char* cursor_from = text->textfilebuf + text->textfilesize;
		char* cursor_to = text->textfilebuf + text->textfilesize + length;
		while (cursor_to > cursor_end) {
			*cursor_to-- = *cursor_from--;
		}
	}

	memcpy(text->textfilebuf + g_view->cursor, (char*)str, length);

	text->textfilesize += length;
	g_view->cursor += length;


	undo_commit();

	//text_insert_char('\n');
	//while (number_of_tabs--) {
	//	text_insert_char('\t');
	//}
}

void text_select_func() {

	int screen_start_backup = g_view->screen_start;
	int cursor_backup = g_view->cursor;
	int text_editor_mode_backup = g_view->text_editor_mode;

	//find the function start
	text_goto_string_start();

	while (true) {
		if (g_view->cursor == 0) break;

		char* cur = text->textfilebuf + g_view->cursor;
		if (*cur == ' ' || *cur == '\t' || *cur == '\n' || *cur == '}') {

		}
		else {
			break;
		}

		//LATER: can be made more efficiently, but eh
		text_up_where(1);
		text_goto_string_start();

	}

	int start = g_view->cursor;

	//now find the function end
	while (true) {
		text_goto_string_end();
		text_right();
		if (g_view->cursor >= text->textfilesize) break;
		char* cur = text->textfilebuf + g_view->cursor;
		if (*cur == '}') break;
	}

	int end = g_view->cursor;

	g_view->text_editor_mode = text_editor_mode_select;
	g_view->cursor = start;
	g_view->selection = end;
	text_copy(g_hwnd);

	g_view->screen_start = screen_start_backup;
	g_view->cursor = cursor_backup;
	g_view->text_editor_mode = text_editor_mode_backup;

}


void text_insert_char(int insert) {

	//can be +1, +15 is just in case...
	//LATER
	if (text->textfilesize + 15 >= text->textfilebufsize) {

		char* nextbuf = (char*)malloc(text->textfilebufsize * 2);

		memcpy(nextbuf, text->textfilebuf, text->textfilesize);

		free(text->textfilebuf);

		text->textfilebuf = nextbuf;

		text->textfilebufsize *= 2;

	}

	char* c;
	if (undo_committed_not == false || undo_committed_not == true && text->undo_current->add_or_del == true) {
		//first time
		undo_commit();
		undo_create(40);
		c = (char*)(text->undo_current + 1);
		text->undo_current->offset = g_view->cursor;
		text->undo_current->length = 0;
		//cursor++;
	}
	else {
		c = (char*)(text->undo_current + 1) + text->undo_current->length;

	}
	int utf8len = 1;
	char* oldc = c;
	if (insert & ~0x7F) {
		//not ascii

		wchar_to_utf8(insert, (u1*&)c);
		utf8len = c - oldc;
		text->undo_current->length += c - oldc;
		//text->undo_current->length++;
	}
	else {
		text->undo_current->length++;
		*c = insert;
	}

	undo_committed_not = true;

	g_display.need_redraw_not = 0;

	//now add the actual symbol into the actual buffer

	//first, shift all chars to the right
	//then, place your char into the freed place

	if (utf8len == 1) {
		char* cursor_end = text->textfilebuf + g_view->cursor;
		char* cursor_from = text->textfilebuf + text->textfilesize;
		char* cursor_to = text->textfilebuf + text->textfilesize + 1;
		while (cursor_to != cursor_end) {
			*cursor_to-- = *cursor_from--;
		}

		*cursor_end = insert;
		g_view->cursor++;
		text->textfilesize++;
	}
	else {
		char* cursor_end = text->textfilebuf + g_view->cursor;
		char* cursor_from = text->textfilebuf + text->textfilesize;
		char* cursor_to = text->textfilebuf + text->textfilesize + utf8len;
		while (cursor_to != cursor_end) {
			*cursor_to-- = *cursor_from--;
		}

		char* c = oldc;
		for (int i = 0; i < utf8len; i++) {
			*cursor_end++ = *c++;
		}
		g_view->cursor += utf8len;
		text->textfilesize += utf8len;

		text_unicode_get_string(oldc, c);

	}
}

void text_insert_string(char* str, int length) {
	undo_commit();
	undo_create(length);
	undo_committed_not = true;
	text->undo_current->offset = g_view->cursor;
	text->undo_current->length = length;
	memcpy((char*)(text->undo_current + 1), (char*)str, length);

	//now copy it into the text buffer

	//allocate more space if necessary
	//first free some space
	//next copy the string in freed space

	if (text->textfilesize + length >= text->textfilebufsize) {

		text->textfilebufsize = next_power_of_2(text->textfilesize + length) * 2;

		char* nextbuf = (char*)malloc(text->textfilebufsize);

		memcpy(nextbuf, text->textfilebuf, text->textfilesize);

		free(text->textfilebuf);

		text->textfilebuf = nextbuf;

	}

	{
		char* cursor_end = text->textfilebuf + g_view->cursor;
		char* cursor_from = text->textfilebuf + text->textfilesize;
		char* cursor_to = text->textfilebuf + text->textfilesize + length;
		while (cursor_from >= cursor_end) {
			*cursor_to-- = *cursor_from--;
		}
	}

	memcpy(text->textfilebuf + g_view->cursor, (char*)str, length);

	text->textfilesize += length;
	g_view->cursor += length;

	undo_commit();

}

void text_backspace() {

	if (g_view->cursor == 0) {
		return;
	}

	char* c;
	if (undo_committed_not == false || undo_committed_not == true && text->undo_current->add_or_del == false) {
		undo_commit();
		undo_create(40);
		text->undo_current->offset = g_view->cursor;
		text->undo_current->length = 0;
		text->undo_current->add_or_del = true;
	}
	else {
		c = (char*)(text->undo_current + 1) + text->undo_current->length;

	}

	//text->undo_current->length++;

	char* cursor_from;
	char* cursor_to;
	char* cursor_end;

	cursor_from = text->textfilebuf + g_view->cursor - 1;
	int charlen = 1;
	while (true) {
		if ((*cursor_from & 0xC0) != 0x80) {
			break;
		}
		charlen++;
		cursor_from--;
	}

	text->undo_current->length += charlen;

	//need to copy deleted chars for later undo
	//first just shift all characters in commit to the right, one (or multiple for utf8) byte to the right

	int length = text->undo_current->length;

	cursor_from = (char*)(text->undo_current + 1) + length - charlen;
	cursor_to = cursor_from + charlen;
	cursor_end = (char*)(text->undo_current + 1);
	while (cursor_from >= cursor_end) {
		*cursor_to-- = *cursor_from--;
	}

	//cursor_from = (char*)(text->undo_current + 1) + length - charlen; //it works, don't ask why
	//cursor_to = cursor_from + 1; // + cursor - length
	//cursor_end = (char*)(text->undo_current + 1);


	//char* cursor_from = (char*)(text->undo_current + 1) + length - 1; //it works, don't ask why
	//char* cursor_to = cursor_from + 1; // + cursor - length
	//char* cursor_end = (char*)(text->undo_current + 1);
	//while (cursor_to > cursor_end) {
	//	*cursor_to-- = *cursor_from--;
	//}

	cursor_from = text->textfilebuf + g_view->cursor - 1;
	//then just copy the current character

	for (int i = charlen; i > 0; i--) {
		*cursor_to-- = *cursor_from--;
	}

	//not now, just see if it works

	undo_committed_not = true;

	g_display.need_redraw_not = 0;

	//now actually delete it from the buffer

	//shift everything to the left

	//LATER: it's probably possible to break everything here by making a fake utf8 character that ends like one but doesn't have a beginning.
	{
		char* cursor_to = text->textfilebuf + g_view->cursor - charlen;
		char* cursor_from = text->textfilebuf + g_view->cursor;
		char* cursor_end = text->textfilebuf + text->textfilesize;
		while (cursor_from != cursor_end) {
			*cursor_to++ = *cursor_from++;
		}
	}

	g_view->cursor -= charlen;
	text->textfilesize -= charlen;

}

void text_select_line() {
	int cursor_backup = g_view->cursor;

	char* cur = text->textfilebuf + cursor_backup;
	if (cur <= text->textfilebuf) {
	}
	else {
		cur--;
		while (true) {

			char c = *cur;
			if (c == '\n') {
				cur++;
				break;
			}
			if (cur <= text->textfilebuf) break;
			cur--;
		}
	}

	g_view->cursor = cur - text->textfilebuf;

	cur = text->textfilebuf + cursor_backup;
	char* cur_end = text->textfilebuf + text->textfilesize;

	while (true) {
		char c = *cur;
		if (c == 0) break;
		cur++;
		if (c == '\n' || cur >= cur_end) break;
	}
	if (*cur == 0) cur--;
	g_view->selection = cur - text->textfilebuf - 1;
}

void text_delete_selection() {
	undo_commit();

	//undo_current->offset = cursor;
	//undo_current->length = 1;



	int min, max;
	if (g_view->cursor > g_view->selection) {
		min = g_view->selection;
		max = g_view->cursor;
		max++;
		if (max > text->textfilesize) max = text->textfilesize;
		if (min == max) return;

		//LATER: probably will break for characters that take 3 or more bytes
		while ((*(text->textfilebuf + max) & 0xC0) == 0x80) {
			max++;
		}

		//if ((*(text->textfilebuf + max) & 0xC0) == 0xC0) {
		//	max++;
		//	while ((*(text->textfilebuf + max) & 0xC0) == 0x80) {
		//		max++;
		//	}
		//	max--;
		//}

		undo_create(max - min);
		text->undo_current->direction = 0;
		text->undo_current->offset = min;
		text->undo_current->length = max - min;
	}
	else {
		min = g_view->cursor;
		max = g_view->selection;
		max++;
		if (max > text->textfilesize) max = text->textfilesize;
		if (min == max) return;

		while ((*(text->textfilebuf + max) & 0xC0) == 0x80) {
			max++;
		}

		//if ((*(text->textfilebuf + max) & 0xC0) == 0xC0) {
		//	max++;
		//	while ((*(text->textfilebuf + max) & 0xC0) == 0x80) {
		//		max++;
		//	}
		//	max--;
		//}

		undo_create(max - min);
		text->undo_current->direction = 1;
		text->undo_current->offset = min;
		text->undo_current->length = max - min;
	}




	text->undo_current->add_or_del = true;
	text->undo_current->is_selection = true;

	//text->undo_current->offset = min;
	//text->undo_current->length = max - min;

	{
		int length = text->undo_current->length;

		char* cursor_from = text->textfilebuf + min; //it works, don't ask why
		char* cursor_end = cursor_from + length;

		char* cursor_to = (char*)(text->undo_current + 1); // + cursor - length

		while (cursor_from < cursor_end) {
			*cursor_to++ = *cursor_from++;
		}
	}

	//cursor = max;
	//undo_committed = false;
	//undo_commit();



	//{
	//	char* cursor_to = textfilebuf + cursor - undo_current->length; // + cursor - length
	//	char* cursor_from = textfilebuf + cursor;
	//	char* cursor_end = textfilebuf + textfilesize;
	//	while (cursor_from != cursor_end) {
	//		*cursor_to++ = *cursor_from++;
	//	}

	//	cursor -= undo_current->length;
	//	textfilesize -= undo_current->length;
	//}


	g_display.need_redraw_not = 0;

	//now actually delete this text
	//shift to the left

	{
		char* cursor_to = text->textfilebuf + min;
		char* cursor_from = text->textfilebuf + max;
		char* cursor_end = text->textfilebuf + text->textfilesize;
		while (cursor_from < cursor_end) {

			*cursor_to++ = *cursor_from++;
		}
	}

	g_view->cursor = min;
	text->textfilesize -= text->undo_current->length;
	undo_commit();
}

void text_copy_string(char* string) {
	if (!OpenClipboard(g_hwnd))
		return;

	EmptyClipboard();

	int len = strlen(string);

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len + 1);
	char* cur = (char*)GlobalLock(hMem);
	memcpy(cur, string, len);
	*(cur + len) = 0;

	GlobalUnlock(hMem);

	SetClipboardData(CF_TEXT, hMem);

	CloseClipboard();
}

void text_copy(HWND hWnd) {
	if (!OpenClipboard(hWnd))
		return;

	EmptyClipboard();

	int min, max;
	if (g_view->cursor > g_view->selection) {
		min = g_view->selection;
		max = g_view->cursor;
	}
	else {
		min = g_view->cursor;
		max = g_view->selection;
	}
	max++;
	if ((*(text->textfilebuf + max) & 0x80) == 0x80) { //in case there is unicode
		max++;
		while ((*(text->textfilebuf + max) & 0xC0) == 0x80) {
			max++;
		}
	}
	if (max > text->textfilesize) {
		max = text->textfilesize;
	}


	int len = MultiByteToWideChar(CP_UTF8, 0, text->textfilebuf + min, max - min, NULL, 0);

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * 2);
	wchar_t* cur = (wchar_t*)GlobalLock(hMem);
	MultiByteToWideChar(CP_UTF8, 0, text->textfilebuf + min, max - min, cur, len);
	*(cur + len) = 0;
	*(cur + len + 1) = 0;


	//char* output = text->textfilebuf + min;
	//const size_t len = max - min;
	//HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len + 1);
	//char* cur = (char*)GlobalLock(hMem);
	//memcpy(cur, output, len);
	//*(cur + len) = 0;

	//remember, it must be null terminated

	GlobalUnlock(hMem);

	SetClipboardData(CF_UNICODETEXT, hMem);

	CloseClipboard();

	g_view->text_editor_mode = text_editor_mode_navigation;
	g_display.need_redraw_not = 0;
}

void text_copy_cut(HWND hWnd) {
	text_copy(hWnd);

	text_delete_selection();
}

void text_copy_paste(HWND hWnd) {
	//paste
	undo_commit();

	if (!IsClipboardFormatAvailable(CF_UNICODETEXT))
		return;

	if (!OpenClipboard(hWnd))
		return;

	HGLOBAL   hglb = GetClipboardData(CF_UNICODETEXT);
	if (hglb != NULL)
	{
		LPTSTR    lptstr = (LPTSTR)GlobalLock(hglb);

		//WString *temp = WString::New();
		//*temp << L"Memory size is " << HeapSize(GetProcessHeap(), 0, lptstr);
		//temp->Alert();
		//since it's wchar, it will show amount_of_characters * 2

		if (lptstr != NULL)
		{

			int length = WideCharToMultiByte(CP_UTF8, 0, lptstr, -1, 0, 0, 0, 0);

			char* utf8str = (char*)malloc(length);
			WideCharToMultiByte(CP_UTF8, 0, lptstr, -1, utf8str, length, 0, 0);

			GlobalUnlock(hglb);
			CloseClipboard();
			length--;
			text_insert_string(utf8str, length);



			text_unicode_get_string(text->textfilebuf + g_view->cursor - length, text->textfilebuf + g_view->cursor);

			free(utf8str);
		}
	}


	g_display.need_redraw_not = 0;
}


void text_scroll_sanitize() {

	const int howmuchtoshift = 3;

	int backup_cursor = g_view->cursor;
	int backup_screen_start = g_view->screen_start;

	bool need_shift_down = false;

	//g_view->screen_start = g_view->cursor;
	text_scroll_down_where(g_view->ysize - 1 - howmuchtoshift);

	if (g_view->screen_start < backup_cursor) {
		//shift
		need_shift_down = true;
	}

	g_view->cursor = backup_cursor;
	g_view->screen_start = backup_screen_start;
	if (need_shift_down) {
		//ugh, i need to know the exact number of lines...
		g_view->screen_start = g_view->cursor;
		text_scroll_up_where(g_view->ysize - 1 - howmuchtoshift);
		return;
	}

	bool need_shift_up = false;

	text_scroll_down_where(howmuchtoshift);
	if (g_view->screen_start >= backup_cursor) {
		//shift
		need_shift_up = true;
	}

	g_view->cursor = backup_cursor;
	g_view->screen_start = backup_screen_start;
	if (need_shift_up) {
		g_view->screen_start = g_view->cursor;
		text_scroll_up_where(howmuchtoshift);
		return;
	}
}

void text_scroll_down_where(int whereshouldigo) {

	if (g_view->screen_start == text->textfilesize) return;
	g_display.need_redraw_not = 0;

	//int whereshouldigo = 2; //where i should go
	//0 for 1 line down
	//2 for 3 lines down
	whereshouldigo--;


	int whereami = -1; //where i am now

	char* cur_osd = text->textfilebuf + g_view->screen_start;
	char* original_location = cur_osd;
	char* last_newline = 0;
	char* lastn = 0;
	//cur_osd--;

	//find the previous newline
	//find if you are 1 or more lines away from it
	//go to the previous line


	while (true) {
		cur_osd--;
		if (cur_osd <= text->textfilebuf) {
			cur_osd = text->textfilebuf;
			break;
		}
		char c = *cur_osd;
		if (c == '\n') {
			cur_osd++;
			break;
		}

	}

	//count the amount of newlines between cur_osd and original_location
	//if 0, go to the previous paragraph
	//else, stay in this paragraph
	//make it more general, it's easier that way


	int current_line = 0;

	while (true) {

		while (true) {

			char* cur_osd_backup = cur_osd;
			int length = 0;
			int last_space = 0;
			char* last_space_cur = 0;
			while (true) {
				char c = *cur_osd;
				cur_osd++;
				length++;

				if (cur_osd > text->textfilebuf + text->textfilesize) {
					cur_osd = cur_osd_backup;
					goto done;
				}
				if (c == '\n') {
					break;
				}
				else if (c == ' ' || c == '\t') {
					if (c == '\t') {
						length += 3;
					}
					last_space = length;
					last_space_cur = cur_osd;
				}
				if (length > g_view->xsize) {
					if (last_space == 0) {
						//THROW();
						break;
					}
					else cur_osd = last_space_cur;
					break;
				}

			}
			current_line++;


			if (whereami < 0) {
				if (cur_osd > original_location) {
					whereami = current_line - 1;
				}
			}
			if (whereami >= 0) {
				if (current_line - 1 == whereami + whereshouldigo) {
					goto done;
				}
			}


			last_newline = cur_osd;

		}
	}


done:

	g_view->screen_start = (int)(cur_osd - text->textfilebuf);



}


void text_scroll_up_where(int whereshouldigo) {
	//almost like text_scroll_down_where
	//first find whereami
	//then iterate till whereshouldigo
	//if not in this paragraph, search previous one

	//if (g_view->screen_start == text->textfilesize) return;
	g_display.need_redraw_not = 0;


	//int whereshouldigo = -3; //where i should go
	//-1 for 1 line
	//-3 for 3 lines
	whereshouldigo = -whereshouldigo;

	int current_line_top = 0;


	char* cur_osd = text->textfilebuf + g_view->screen_start;
	char* original_location = cur_osd;

	while (true) {
		cur_osd--;
		if (cur_osd <= text->textfilebuf) {
			cur_osd = text->textfilebuf;
			break;
		}
		char c = *cur_osd;
		if (c == '\n') {
			cur_osd++;
			break;
		}

	}

	//count the amount of newlines between cur_osd and original_location
	//if 0, go to the previous paragraph
	//else, stay in this paragraph


	//1) find whereami

	int whereami;
	int current_line = 0;
	char* cur_osd_backup_backup = cur_osd;

	while (true) {

		char* cur_osd_backup = cur_osd;
		int length = 0;
		int last_space = 0;
		char* last_space_cur = 0;
		while (true) {
			char c = *cur_osd;
			cur_osd++;
			length++;

			if (cur_osd > text->textfilebuf + text->textfilesize) {
				//cur_osd = cur_osd_backup;
				//goto done;
				break;
			}
			if (c == '\n') {
				break;
			}
			else if (c == ' ' || c == '\t') {
				if (c == '\t') {
					length += 3;
				}
				last_space = length;
				last_space_cur = cur_osd;
			}
			if (length > g_view->xsize) {
				if (last_space == 0) {
					//THROW();
					break;
				}
				else cur_osd = last_space_cur;
				break;
			}
		}

		if (cur_osd > original_location) {
			whereami = current_line;

			cur_osd = cur_osd_backup_backup;



			goto goto_whereami;
		}

		current_line++;

	}

goto_whereami:

	if (whereami + whereshouldigo == 0) goto done;

	else if (whereami + whereshouldigo > 0) {
		//seek in this paragraph

		char* cur_osd_backup_backup = cur_osd;
		int current_line = 0;

		while (true) {


			char* cur_osd_backup = cur_osd;
			int length = 0;
			int last_space = 0;
			char* last_space_cur = 0;
			while (true) {
				char c = *cur_osd;
				cur_osd++;
				length++;

				if (cur_osd > text->textfilebuf + text->textfilesize) {
					//cur_osd = cur_osd_backup;
					//goto done;
					break;
				}
				if (c == '\n') {
					break;
				}
				else if (c == ' ' || c == '\t') {
					if (c == '\t') {
						length += 3;
					}
					last_space = length;
					last_space_cur = cur_osd;
				}
				if (length > g_view->xsize) {
					if (last_space == 0) {
						//THROW();
						break;
					}
					else cur_osd = last_space_cur;
					break;
				}

			}

			if (whereami + whereshouldigo == current_line) {
				cur_osd = cur_osd_backup;
				goto done;
			}

			current_line++;

		}

	}

	//seek in previous paragraph

	whereshouldigo += whereami;

	while (true) {

		//go to the previous paragraph

		if (cur_osd == text->textfilebuf) goto done;

		cur_osd--;
		while (true) {
			cur_osd--;
			if (cur_osd <= text->textfilebuf) {
				cur_osd = text->textfilebuf;
				//break;
				goto done;
			}
			char c = *cur_osd;
			if (c == '\n') {
				cur_osd++;
				break;
			}
		}

		//goto done;







		//count lines

		int current_line = 0;
		char* cur_osd_backup_backup = cur_osd;

		while (true) {


			char* cur_osd_backup = cur_osd;
			int length = 0;
			int last_space = 0;
			char* last_space_cur = 0;
			while (true) {
				char c = *cur_osd;
				cur_osd++;
				length++;

				if (c == '\n') {
					current_line++;
					goto find_the_needed_line;
					//break;
				}
				else if (c == ' ' || c == '\t') {
					if (c == '\t') {
						length += 3;
					}
					last_space = length;
					last_space_cur = cur_osd;
				}
				if (length > g_view->xsize) {
					if (last_space == 0) {
						//THROW();
						break;
					}
					else cur_osd = last_space_cur;
					break;
				}

			}
			current_line++;
		}

	find_the_needed_line:

		int lines_total = current_line;
		cur_osd = cur_osd_backup_backup;

		current_line_top -= lines_total;

		if (current_line_top > whereshouldigo) {
			continue;
		}


		//find the needed line

		current_line = current_line_top;
		cur_osd_backup_backup = cur_osd;

		while (true) {


			char* cur_osd_backup = cur_osd;
			int length = 0;
			int last_space = 0;
			char* last_space_cur = 0;
			while (true) {
				char c = *cur_osd;
				cur_osd++;
				length++;


				if (c == '\n') {
					//whereshouldigo += current_line;
					//current_line++;
					break;
					//break;
				}
				else if (c == ' ' || c == '\t') {
					if (c == '\t') {
						length += 3;
					}
					last_space = length;
					last_space_cur = cur_osd;
				}
				if (length > g_view->xsize) {
					if (last_space == 0) {
						//THROW();
						break;
					}
					else cur_osd = last_space_cur;
					break;
				}

			}
			current_line++;
			if (current_line - 1 == whereshouldigo) {
				cur_osd = cur_osd_backup;
				goto done;
			}

		}


		//whereshouldigo += lines_total;

	}


done:

	g_view->screen_start = (int)(cur_osd - text->textfilebuf);

}





void text_down_where(int whereshouldigo) {

	if (g_view->cursor == text->textfilesize) return;
	g_display.need_redraw_not = 0;
	undo_commit();

	//int whereshouldigo = 2; //where i should go
	//0 for 1 line down
	//2 for 3 lines down
	whereshouldigo--;


	int whereami = -1; //where i am now
	int x = 0; //like whereami, but char to the right.

	char* cur_osd = text->textfilebuf + g_view->cursor;
	char* original_location = cur_osd;
	char* last_newline = 0;
	char* lastn = 0;
	//cur_osd--;

	//find the previous newline
	//find if you are 1 or more lines away from it
	//go to the previous line


	while (true) {
		cur_osd--;
		if (cur_osd < text->textfilebuf) {
			cur_osd = text->textfilebuf;
			break;
		}
		char c = *cur_osd;
		if (c == '\n') {
			cur_osd++;
			break;
		}

	}

	//count the amount of newlines between cur_osd and original_location
	//if 0, go to the previous paragraph
	//else, stay in this paragraph
	//make it more general, it's easier that way


	int current_line = 0;

	while (true) {

		while (true) {

			char* cur_osd_backup = cur_osd;
			int length = 0;
			int last_space = 0;
			char* last_space_cur = 0;
			while (true) {
				char c = *cur_osd;
				cur_osd++;
				length++;

				if (cur_osd > text->textfilebuf + text->textfilesize) {
					cur_osd = cur_osd_backup;
					goto done;
				}
				if (c == '\n') {
					break;
				}
				else if (c == ' ' || c == '\t') {
					if (c == '\t') {
						length += 3;
					}
					last_space = length;
					last_space_cur = cur_osd;
				}
				if (length > g_view->xsize) {
					if (last_space == 0) {
						//THROW();
						break;
					}
					else cur_osd = last_space_cur;
					break;
				}

			}
			current_line++;


			if (whereami < 0) {
				if (cur_osd > original_location) {
					whereami = current_line - 1;
					x = original_location - cur_osd_backup;
				}
			}
			if (whereami >= 0) {
				if (current_line - 1 == whereami + whereshouldigo) {
					goto done;
				}
			}


			last_newline = cur_osd;

		}
	}


done:


	//cursor += x;
	//if destination line is shorter than x, then go to the next \n instead
	while (x--) {
		char c = *cur_osd++;
		if (c == '\n') {
			cur_osd--;
			break;
		}
	}

	g_view->cursor = (int)(cur_osd - text->textfilebuf);

	text_scroll_sanitize();

}

void text_up_where(int whereshouldigo) {
	//almost like text_scroll_down_where
	//first find whereami
	//then iterate till whereshouldigo
	//if not in this paragraph, search previous one

	if (g_view->cursor > text->textfilesize) return;
	g_display.need_redraw_not = 0;

	undo_commit();


	//int whereshouldigo = -3; //where i should go
	//-1 for 1 line
	//-3 for 3 lines
	whereshouldigo = -whereshouldigo;

	int current_line_top = 0;


	char* cur_osd = text->textfilebuf + g_view->cursor;
	char* original_location = cur_osd;

	while (true) {
		cur_osd--;
		if (cur_osd <= text->textfilebuf) {
			cur_osd = text->textfilebuf;
			break;
		}
		char c = *cur_osd;
		if (c == '\n') {
			cur_osd++;
			break;
		}

	}

	//count the amount of newlines between cur_osd and original_location
	//if 0, go to the previous paragraph
	//else, stay in this paragraph


	//1) find whereami

	int whereami;
	int x = 0; //like whereami, but char to the right.
	int current_line = 0;
	char* cur_osd_backup_backup = cur_osd;

	while (true) {

		char* cur_osd_backup = cur_osd;
		int length = 0;
		int last_space = 0;
		char* last_space_cur = 0;
		while (true) {
			char c = *cur_osd;
			cur_osd++;
			length++;

			if (cur_osd >= text->textfilebuf + text->textfilesize) {
				//cur_osd = cur_osd_backup;
				//goto done;
				break;
			}
			if (c == '\n') {
				break;
			}
			else if (c == ' ' || c == '\t') {
				if (c == '\t') {
					length += 3;
				}
				last_space = length;
				last_space_cur = cur_osd;
			}
			if (length > g_view->xsize) {
				if (last_space == 0) {
					//THROW();
					break;
				}
				else cur_osd = last_space_cur;
				break;
			}
		}

		if (cur_osd > original_location) {
			whereami = current_line;
			x = original_location - cur_osd_backup;

			cur_osd = cur_osd_backup_backup;



			goto goto_whereami;
		}

		current_line++;

	}

goto_whereami:

	if (whereami + whereshouldigo == 0) goto done;

	else if (whereami + whereshouldigo > 0) {
		//seek in this paragraph

		char* cur_osd_backup_backup = cur_osd;
		int current_line = 0;

		while (true) {


			char* cur_osd_backup = cur_osd;
			int length = 0;
			int last_space = 0;
			char* last_space_cur = 0;
			while (true) {
				char c = *cur_osd;
				cur_osd++;
				length++;

				if (cur_osd >= text->textfilebuf + text->textfilesize) {
					//cur_osd = cur_osd_backup;
					//goto done;
					break;
				}
				if (c == '\n') {
					break;
				}
				else if (c == ' ' || c == '\t') {
					if (c == '\t') {
						length += 3;
					}
					last_space = length;
					last_space_cur = cur_osd;
				}
				if (length > g_view->xsize) {
					if (last_space == 0) {
						//THROW();
						break;
					}
					else cur_osd = last_space_cur;
					break;
				}

			}

			if (whereami + whereshouldigo == current_line) {
				cur_osd = cur_osd_backup;
				goto done;
			}

			current_line++;

		}

	}

	//seek in previous paragraph

	whereshouldigo += whereami;

	while (true) {

		//go to the previous paragraph

		if (cur_osd == text->textfilebuf) goto done;

		cur_osd--;
		while (true) {
			cur_osd--;
			if (cur_osd < text->textfilebuf) {
				cur_osd = text->textfilebuf;
				//break;
				goto done;
			}
			char c = *cur_osd;
			if (c == '\n') {
				cur_osd++;
				break;
			}
		}

		//goto done;







		//count lines

		int current_line = 0;
		char* cur_osd_backup_backup = cur_osd;

		while (true) {


			char* cur_osd_backup = cur_osd;
			int length = 0;
			int last_space = 0;
			char* last_space_cur = 0;
			while (true) {
				char c = *cur_osd;
				cur_osd++;
				length++;

				if (c == '\n') {
					current_line++;
					goto find_the_needed_line;
					//break;
				}
				else if (c == ' ' || c == '\t') {
					if (c == '\t') {
						length += 3;
					}
					last_space = length;
					last_space_cur = cur_osd;
				}
				if (length > g_view->xsize) {
					if (last_space == 0) {
						//THROW();
						break;
					}
					else cur_osd = last_space_cur;
					break;
				}

			}
			current_line++;
		}

	find_the_needed_line:

		int lines_total = current_line;
		cur_osd = cur_osd_backup_backup;

		current_line_top -= lines_total;

		if (current_line_top > whereshouldigo) {
			continue;
		}


		//find the needed line

		current_line = current_line_top;
		cur_osd_backup_backup = cur_osd;

		while (true) {


			char* cur_osd_backup = cur_osd;
			int length = 0;
			int last_space = 0;
			char* last_space_cur = 0;
			while (true) {
				char c = *cur_osd;
				cur_osd++;
				length++;


				if (c == '\n') {
					//whereshouldigo += current_line;
					//current_line++;
					break;
					//break;
				}
				else if (c == ' ' || c == '\t') {
					if (c == '\t') {
						length += 3;
					}
					last_space = length;
					last_space_cur = cur_osd;
				}
				if (length > g_view->xsize) {
					if (last_space == 0) {
						//THROW();
						break;
					}
					else cur_osd = last_space_cur;
					break;
				}

			}
			current_line++;
			if (current_line - 1 == whereshouldigo) {
				cur_osd = cur_osd_backup;
				goto done;
			}

		}


		//whereshouldigo += lines_total;

	}


done:

	//cursor += x;
	//if destination line is shorter than x, then go to the next \n instead
	while (x--) {
		char c = *cur_osd++;
		if (c == '\n') {
			cur_osd--;
			break;
		}
	}

	g_view->cursor = (int)(cur_osd - text->textfilebuf);

	text_scroll_sanitize();

}




void text_tabify_and_fix_newlines() {
	//if it sees 3 spaces, it will add 0 tabs, spaces discarded completely
	//if it sees 4 spaces, it will add 1 tab
	g_view->cursor = 0;
	char* cur_src = text->textfilebuf;
	char* cur_dst = text->textfilebuf;
	char* cur_end = text->textfilebuf + text->textfilesize;
	while (true) {
		char* cur_src_linestart = cur_src;
		char* cur_dst_linestart = cur_dst;
		while (true)
		{
			for (int i = 0; i < 4; i++) {
				if (cur_src == cur_end) goto spaces_ended;
				if (*cur_src != ' ') goto spaces_ended;
				cur_src++;
			}
			cur_dst++;
		}
	spaces_ended:
		while (cur_dst_linestart < cur_dst) *cur_dst_linestart++ = '\t';

		char* cur_src_old = cur_src;
		char* cur_dst_old = cur_dst;
		while (true)
		{
			if (cur_src == cur_end) goto line_ended;
			if (*cur_src == '\n') goto line_ended;
			while ((*cur_src & 0xC0) == 0x80) {
				cur_src++;
			}
			cur_src++;
		}
		//find out how many spaces at the beginning of the line are there
		//replace newline
	line_ended:
		while (cur_src_old < cur_src) {
			if (*cur_src_old == '\r') {
				cur_src_old++;
				continue;
			}
			while ((*cur_src_old & 0xC0) == 0x80) {
				*cur_dst_old++ = *cur_src_old++;
			}
			*cur_dst_old++ = *cur_src_old++;
		}
		cur_src = cur_src_old;
		cur_dst = cur_dst_old;

		if (cur_src >= cur_end) break;

		if (*cur_src == '\n') {
			*cur_dst++ = *cur_src++;
		}

		if (cur_src >= cur_end) break;
	}
	text->textfilesize = cur_dst - text->textfilebuf;
}

void text_left() {
	undo_commit();
	g_view->cursor--;
	char* cursor_offset = text->textfilebuf + g_view->cursor;
	while ((*cursor_offset & 0xC0) == 0x80) {
		cursor_offset--;
		g_view->cursor--;
	}

	if (g_view->cursor < 0) g_view->cursor = 0;

	g_display.need_redraw_not = 0;
}

void text_right() {
	undo_commit();
	g_view->cursor++;
	char* cursor_offset = text->textfilebuf + g_view->cursor;
	while ((*cursor_offset & 0xC0) == 0x80) {
		cursor_offset++;
		g_view->cursor++;
	}

	if (g_view->cursor >= text->textfilesize) g_view->cursor = text->textfilesize;

	g_display.need_redraw_not = 0;
}




void search() {

	char* cur_start = text->textfilebuf + g_view->cursor + 1;
	char* cur_end = text->textfilebuf + text->textfilesize - search_string_used;

	bool found = false;

	while (cur_start < cur_end) {
		if (memcmp(cur_start, search_string, search_string_used) == 0) {
			found = true;
			break;
		}

		cur_start++;
	}

	if (found) {
		g_view->cursor = cur_start - text->textfilebuf;
		text_scroll_sanitize();

		g_view->text_editor_mode = text_editor_mode_select;

		g_view->selection = g_view->cursor + search_string_used - 1;
	}
}



void search_stay() {

	char* cur_start = text->textfilebuf + g_view->cursor;
	char* cur_end = text->textfilebuf + text->textfilesize - search_string_used;

	bool found = false;

	while (cur_start < cur_end) {
		if (memcmp(cur_start, search_string, search_string_used) == 0) {
			found = true;
			break;
		}

		cur_start++;
	}

	if (found) {
		g_view->cursor = cur_start - text->textfilebuf;
		text_scroll_sanitize();

		g_view->text_editor_mode = text_editor_mode_select;

		g_view->selection = g_view->cursor + search_string_used - 1;
	}
}

void search_back() {

	char* cur_start = text->textfilebuf + g_view->cursor - 1;
	char* cur_end = text->textfilebuf;

	bool found = false;

	while (cur_start >= cur_end) {
		if (memcmp(cur_start, search_string, search_string_used) == 0) {
			found = true;
			break;
		}

		cur_start--;
	}

	if (found) {
		g_view->cursor = cur_start - text->textfilebuf;
		text_scroll_sanitize();

		g_view->text_editor_mode = text_editor_mode_select;

		g_view->selection = g_view->cursor + search_string_used - 1;
	}
}

void RenderText() {

	if (multithreading)
		EnterCriticalSection(&warn_mutex);

	if (warn_tickcount) {
		if (GetTickCount() > warn_tickcount) {
			warn_tickcount = 0;
			for (int i = 0; i < text_warn_max; i++) {
				text_warn[i] = 0;
			}
		}
	}

	if (multithreading)
		LeaveCriticalSection(&warn_mutex);

	char* cur_osd; //because goto draw_warn jumps over declaration of cur_osd

	const int vertices_size = vertex_buffer_capacity * vertex_buffer_capacity_mul * sizeof(CUSTOMVERTEX);

	struct VertexCursor {
		TextureAndBuffer* texture;
		uint32_t quads;
		uint32_t last_buf;
		float* p;

		void Start() {
			if (texture->buffers[0] == 0) {
				if (FAILED(d3dDevice->CreateVertexBuffer(vertex_buffer_capacity * vertex_buffer_capacity_mul * sizeof(CUSTOMVERTEX),
					D3DUSAGE_DYNAMIC, D3DFVF_CUSTOMVERTEX,
					D3DPOOL_DEFAULT, &(texture->buffers[0]), NULL)))
				{
					THROW();
				}
			}

			void* pVertices;
			if (FAILED(texture->buffers[0]->Lock(0, vertices_size, &pVertices, D3DLOCK_DISCARD)))
				THROW();
			p = (float*)pVertices;
		}

		void fill_list(float where_l, float where_r, float where_t, float where_b, float what_l, float what_r, float what_t, float what_b) {
			where_l += 0.5;
			where_r += 0.5;
			where_t += 0.5;
			where_b += 0.5;

			//where_r -= 1;
			////what_r -= 1 / ;

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

			quads++;
			Sanitize();
		}

		void Sanitize() {
			if (quads == vertex_buffer_capacity) {
				Unlock();

				quads = 0;
				last_buf++;
				if (last_buf == buffers_max) THROW("text out of space");

				if (!texture->buffers[last_buf]) {

					if (FAILED(d3dDevice->CreateVertexBuffer(vertex_buffer_capacity * vertex_buffer_capacity_mul * sizeof(CUSTOMVERTEX),
						D3DUSAGE_DYNAMIC, D3DFVF_CUSTOMVERTEX,
						D3DPOOL_DEFAULT, &(texture->buffers[last_buf]), NULL)))
					{
						THROW();
					}
				}

				VOID* pVertices;
				if (FAILED(texture->buffers[last_buf]->Lock(0, vertices_size, (void**)&pVertices, D3DLOCK_DISCARD))) {
					THROW();
				}

				p = (float*)pVertices;

			}
		}

		void Unlock() {
			texture->buffers[last_buf]->Unlock();
		}

		void Render() {

			if (quads == 0 && last_buf == 0) return;

			//
			//g_texture_text
			int res = d3dDevice->SetTexture(0, texture->texture);
			if (res != D3D_OK) THROW("d3dDevice->SetTexture");


			for (int j = 0; j < last_buf; j++) {
				d3dDevice->SetStreamSource(0, texture->buffers[j], 0, sizeof(CUSTOMVERTEX));

				d3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, vertex_buffer_capacity * 2);
			}

			if (quads) {
				d3dDevice->SetStreamSource(0, texture->buffers[last_buf], 0, sizeof(CUSTOMVERTEX));

				d3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, quads * 2);
			}
		}
	};

	VertexCursor ascii = { &text_textureandbuffer, 0, 0 };
	VertexCursor unicode = { &unicode_textureandbuffer, 0, 0 };




	const int char_width = 9;
	const int char_height = 16;


	ascii.Start();
	unicode.Start();

	float where_x, where_y, where_w, where_h;
	float what_x, what_y, what_w, what_h;


	//Use this to show the whole texture
	//ascii.fill_list(0.0f, 0.0f + 512.0f,
	//	0.0f, 0.0f + 512.0f,
	//	0.0f, 1.0f,
	//	0.0f, 1.0f);

	//unicode.fill_list(0.0f, 0.0f + 512.0f,
	//		0.0f, 0.0f + 512.0f,
	//		0.0f, 1.0f,
	//		0.0f, 1.0f);



	//THIS IS THE TEXT EDITOR

	where_x = 0.0f;
	where_y = 0.0f;

	const float lineheight_default = 16.0;
	float lineheight = lineheight_default;


	cur_osd = text->textfilebuf + g_view->screen_start;

	if (text_editor_enabled) {

		bool draw_cursor_line_pos_stored = false;
		float draw_cursor_line_pos_where_x, draw_cursor_line_pos_where_y;

		bool last_was_n = false;
		bool last_was_t = false;
		bool last_was_end = false;

		struct text_view* l_view_start = g_view;
		struct text_view* l_view = g_view;

		while (true) {

			int current_line = 0;

			struct text_buffer* text = l_view->text_pointer;

			char* cur_wrap = 0;

			char* cur_osd_end = text->textfilebuf + text->textfilesize;

			char* cursor_offset = text->textfilebuf + l_view->cursor;

			char* commit_start = 0;
			int commit_length = 0;
			bool wait_for_commit = false;
			bool in_commit = false;
			char* cur_osd_backup = NULL;

			bool wait_for_inverse_color = false;
			char* cur_inverse_color_start = 0;
			char* cur_inverse_color_end = 0;
			if (l_view->text_editor_mode == text_editor_mode_select) {
				wait_for_inverse_color = true;
				int min, max;
				if (l_view->cursor > l_view->selection) {
					min = l_view->selection;
					max = l_view->cursor;
				}
				else {
					min = l_view->cursor;
					max = l_view->selection;
				}
				max++;
				cur_inverse_color_start = text->textfilebuf + min;
				cur_inverse_color_end = text->textfilebuf + max;
			}

			while (true) {

				char c = ' ';

				bool draw_cursor_line = false;

				if (last_was_end) {
					break;
				}

				if (cur_osd == cur_osd_end) {
					last_was_end = true;
				}

				bool inverse_color = false;
				bool may_skip = false;

				if (last_was_n || cur_wrap == cur_osd) {
					where_x = 0.0f;
					where_y += lineheight;
					lineheight = lineheight_default;
					last_was_n = false;
					cur_wrap = 0;
					current_line++;
					if (current_line == l_view->ysize) break;
				}
				else if (last_was_t) {
					where_x += 9.0f * 4;
					last_was_t = false;
				}

				if (wait_for_commit) {
					if (in_commit == false) {
						if (cur_osd == commit_start) {
							//found the commit start
							if (text->undo_current->add_or_del) {
								wait_for_commit = false;
								cur_osd += text->undo_current->length;
							}
							else {
								cur_osd_backup = cur_osd;
								cur_osd = (char*)(text->undo_current + 1);
								commit_length--;
								in_commit = true;
							}
						}
					}
					else {
						if (commit_length == 0) {
							cur_osd = cur_osd_backup;
							//in_commit = false;
							wait_for_commit = false;
						}
						else {
							commit_length--;
						}
					}
				}

				if (!last_was_end) {
					c = *cur_osd;
				}

				if (wait_for_inverse_color) {
					if (cur_osd >= cur_inverse_color_start) {
						inverse_color = true;
					}
					if (cur_osd >= cur_inverse_color_end) {
						inverse_color = false;
						wait_for_inverse_color = false;
					}
				}


				if (cur_osd == cursor_offset) {
					if (l_view->text_editor_mode == text_editor_mode_edit) {
						draw_cursor_line = true;
					}
					else if (l_view->text_editor_mode == text_editor_mode_navigation) {
						inverse_color = true;
					}
				}

				if (draw_cursor_line) {

					draw_cursor_line_pos_where_x = where_x;
					draw_cursor_line_pos_where_y = where_y;
					draw_cursor_line_pos_stored = true;
				}



				if (cur_wrap == 0) {
					char* cur_osd_backup = cur_osd;
					int length = 0;
					int last_space = 0;
					char* last_space_cur = 0;
					while (true) {
						char c = *cur_osd;
						cur_osd++;
						length++;
						if (c == '\n') {
							cur_wrap = cur_osd;
							break;
						}
						if (cur_osd == cur_osd_end) {
							cur_wrap = cur_osd + 1;
							break;
						}
						else if (c == ' ' || c == '\t') {
							if (c == '\t') {
								length += 3;
							}
							last_space = length;
							last_space_cur = cur_osd;
						}
						if (length > l_view->xsize) {
							if (last_space == 0) {
								cur_wrap = cur_osd;
							}
							else cur_wrap = last_space_cur;
							break;
						}

					}
					cur_osd = cur_osd_backup;
				}

				//if (!c) break;
				if (c == '\n') {
					last_was_n = true;
					c = ' ';
					may_skip = true;
				}
				else if (c == '\t') {
					last_was_t = true;
					c = ' ';
					may_skip = true;
				}

				cur_osd++;
				if (may_skip && !inverse_color) continue;


				if (where_y >= g_window_h_f) {
					break;
				}


				if ((c & 0xC0) == 0xC0) {
					//do {
					//	c = *cur_osd;
					//	cur_osd++;
					//} while ((c & 0xC0) == 0x80);
					//c = '?';
					//cur_osd--;
					//inverse_color = ~inverse_color;

					cur_osd--;
					wchar_t w = utf8_to_wchar(cur_osd);



					u4 index = unicode_chars->IndexOf(w);
					UnicodeCoord* coord = unicodeCoord->Get(index);


					what_x = coord->x / 1024.0;
					what_y = coord->y / 1024.0;
					what_w = what_x + ((coord->w - 1) / 1024.0);
					what_h = what_y + (coord->h / 1024.0);


					where_w = where_x + coord->w;
					where_h = where_y + coord->h;

					if (inverse_color) {
						what_x += (coord->w / 1024.0);
						what_w += (coord->w / 1024.0);
					}

					unicode.fill_list(where_x, where_w,
						where_y, where_h,
						what_x, what_w,		//should be something like what_x/1024.0f
						what_y, what_h);

					if (!last_was_t) {
						where_x += coord->w;
					}

					lineheight = unicode_texture_fontsize;

				}
				else {

					int whatchar_y = (int)c / 32;
					int whatchar_x = (int)c % 32;

					what_x = whatchar_x * 9.0f / 512.0f;
					what_y = whatchar_y * 16.0f / 512.0f;
					what_w = what_x + (9.0f / 512.0f);
					what_h = what_y + (16.0f / 512.0f);

					if (inverse_color) {
						what_y += (8.0f * 16.0f) / 512.0f;
						what_h += (8.0f * 16.0f) / 512.0f;
					}



					where_w = where_x + 9.0f;
					where_h = where_y + 16.0f;

					ascii.fill_list(where_x, where_w,
						where_y, where_h,
						what_x, what_w,		//should be something like what_x/1024.0f
						what_y, what_h);

					if (!last_was_t) {
						where_x += 9.0f;
					}

				}


			}

			l_view = l_view->view_next;
			if (l_view == l_view_start) break;
		}

		if (draw_cursor_line_pos_stored) {
			ascii.fill_list(draw_cursor_line_pos_where_x - 1.0f, draw_cursor_line_pos_where_x + 1.0f,
				draw_cursor_line_pos_where_y, draw_cursor_line_pos_where_y + 16.0f,
				0.0f, 9.0f / 512.0f,		//should be something like what_x/1024.0f
				(8.0f * 16.0f) / 512.0f, (9.0f * 16.0f) / 512.0f);
		}
		//LATER
		//can overflow g_text_vertexbuffer

	}

	//draw search string
	if (search_mode) {

		bool first_char = true;
		int chars_left = search_string_used;

		where_x = 0.0f;
		where_y = char_h_f * (g_window_h_char - 1);

		cur_osd = search_string;

		while (true) {
			char c;
			if (first_char) {
				first_char = false;
				c = '/';
			}
			else {
				if (chars_left <= 0) break;
				c = *cur_osd;
				if (!c) break;
				cur_osd++;
				chars_left--;
			}

			int whatchar_y = (int)c / 32;
			int whatchar_x = (int)c % 32;

			what_x = whatchar_x * 9.0f / 512.0f;
			what_y = whatchar_y * 16.0f / 512.0f;
			what_w = what_x + (9.0f / 512.0f);
			what_h = what_y + (16.0f / 512.0f);

			where_w = where_x + 9.0f;
			where_h = where_y + 16.0f;

			ascii.fill_list(where_x, where_w,
				where_y, where_h,
				what_x, what_w,		//should be something like what_x/1024.0f
				what_y, what_h);

			where_x += 9.0f;

		}
	}


	ascii.Unlock();
	unicode.Unlock();

draw_warn:


actually_draw:

	ascii.Render();
	unicode.Render();

}


LRESULT Text_MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (text_editor_enabled) {
		//if (msg == WM_KEYDOWN) {
		//	if (wParam == 'F') {
		//		if (GetKeyState(VK_CONTROL) & 0x8000) {
		//			text_editor_enabled = !text_editor_enabled;
		//			g_display.need_redraw_not = 0;
		//			return 0;
		//		}
		//	}
		//}



		if (search_mode) {
			switch (msg)
			{

			case WM_CHAR:
				if (GetKeyState(VK_CONTROL) & 0x8000) {
					//HIBYTE(VkKeyScan((TCHAR)wParam)) & 2
					//ctrl pressed
					//if ((wParam & 0x3f) == ('Z' & 0x3f)) { THROW(); }
					//THROW();
					//eeeh, doesn't work anyway
					//LATER
					//GetKeyState(VK_CONTROL)


					break;
				}
				else {
					switch (wParam)
					{
					case 0x08:

						// Process a backspace.

						break;

					case 0x1B:

						// Process an escape.

						break;

					case 0x0A:
						THROW(); //never happens for me for some reason, I'm only getting 0D

								 // Process a linefeed.
								 // this is the \n

								 //break;

					case 0x0D:  //fallthrough

								// Process a carriage return \r. 

								//wParam = '\n'; //a little hack
								//break;

						search_mode = false;
						g_display.need_redraw_not = 0;
						return 0;


					case 0x09: //fallthrough

							   // Process a tab. 

							   //break;

					default:
						if (search_string_used < search_string_max) {
							char* out = search_string + search_string_used;
							*out = wParam;

							search_string_used++;
							g_display.need_redraw_not = 0;

						}
						//text_insert_char(wParam);

						search_stay();

						return 0;

					}
				}

				return 0;

			case WM_KEYDOWN:
				if (GetKeyState(VK_CONTROL) & 0x8000) {
					//if (wParam == 'Z') {
					//	if (GetKeyState(VK_SHIFT) & 0x8000) {
					//		undo_redo();
					//	}
					//	else {
					//		undo();
					//	}
					//	break;
					//}
					//if (wParam == 'V') {
					//	text_copy_paste(hWnd);
					//}

					if (wParam == 'V') {

						if (!IsClipboardFormatAvailable(CF_TEXT))
							return 0;

						if (!OpenClipboard(hWnd))
							return 0;

						int length = 0;

						HGLOBAL   hglb = GetClipboardData(CF_TEXT);
						if (hglb != NULL)
						{
							LPTSTR    lptstr = (LPTSTR)GlobalLock(hglb);
							if (lptstr != NULL)
							{

								char* cur_in = (char*)lptstr;

								for (; length != search_string_max; length++) {

									char c = *cur_in++;

									if (!c) break;
								}

								if (length > search_string_max) length = search_string_max;

								memcpy(search_string, (char*)lptstr, length);
								*(search_string + length) = 0;

								search_string_used = length;

								GlobalUnlock(hglb);
							}
						}
						CloseClipboard();

						search_stay();

						g_display.need_redraw_not = 0;

					}

				}
				else if (wParam == VK_ESCAPE) {
					search_mode = false;
					g_display.need_redraw_not = 0;
				}

				if (wParam == VK_BACK) {
					if (search_string_used > 0) search_string_used--;
					g_display.need_redraw_not = 0;
				}



			}

		}
		else
		{

			//navigation or selection
			if (g_view->text_editor_mode == text_editor_mode_navigation || g_view->text_editor_mode == text_editor_mode_select) {

				if (msg == WM_KEYDOWN) {
					if (wParam == 'G') {
						if (GetKeyState(VK_SHIFT) & 0x8000) {
							g_view->cursor = text->textfilesize;
							text_scroll_sanitize();
							//text_scroll_up_where(6);
							//text_up_where(0);

						}
						else {


							g_view->cursor = 0;
							g_view->screen_start = 0;
						}
						g_display.need_redraw_not = 0;
					}

					else if (wParam == VK_OEM_COMMA) { //'<' //','
						search_back();
					}

					else if (wParam == VK_OEM_PERIOD) { //'>' //'.'

						search();

						return 0;

					}

					else if (wParam == 'E') {
						text_goto_string_start_skiptabs();

					}
					else if (wParam == 'R') {
						text_goto_string_end();
					}
					else if (wParam == 'W') {
						textfile_save();
					}
				}
			}

			//warn(int_to_string(text_editor_mode));

			//edit mode
			if (g_view->text_editor_mode == text_editor_mode_edit) {
				switch (msg)
				{
				case WM_CHAR:
					if (GetKeyState(VK_CONTROL) & 0x8000) {
						//HIBYTE(VkKeyScan((TCHAR)wParam)) & 2
						//ctrl pressed
						//if ((wParam & 0x3f) == ('Z' & 0x3f)) { THROW(); }
						//THROW();
						//eeeh, doesn't work anyway
						//LATER
						//GetKeyState(VK_CONTROL)

						break;
					}
					else {
						switch (wParam)
						{
						case 0x08:

							// Process a backspace.

							break;

						case 0x1B:

							// Process an escape.

							break;

						case 0x0A:
							THROW(); //never happens for me for some reason, I'm only getting 0D

									 // Process a linefeed.
									 // this is the \n

									 //break;

						case 0x0D:  //fallthrough

									//// Process a carriage return \r. 

									//wParam = '\n'; //a little hack
									////break;
							text_insert_newline_and_tabs();
							return 0;



						case 0x09: //fallthrough

								   // Process a tab. 

								   //break;

						default:

							text_insert_char(wParam);
							return 0;

						}
					}

					return 0;

				case WM_KEYDOWN:
					if (GetKeyState(VK_CONTROL) & 0x8000) {
						if (wParam == 'Z') {
							if (GetKeyState(VK_SHIFT) & 0x8000) {
								undo_redo();
							}
							else {
								undo();
							}
							break;
						}
						if (wParam == 'V') {
							text_copy_paste(hWnd);
						}
					}
					else if (wParam == VK_LEFT) {
						text_left();

						break;
					}
					else if (wParam == VK_RIGHT) {
						text_right();

						break;
					}
					else if (wParam == VK_UP) {
						text_up_where(1);

						break;
					}
					else if (wParam == VK_DOWN) {
						text_down_where(1);

						break;
					}
					else if (wParam == VK_ESCAPE) {
						undo_commit();
						g_view->text_editor_mode = text_editor_mode_navigation;
						g_display.need_redraw_not = 0;
					}

					if (wParam == VK_BACK) {
						text_backspace();
					}



				}
			}
			//navigation mode
			else if (g_view->text_editor_mode == text_editor_mode_navigation) {


				switch (msg)
				{
				case WM_KEYDOWN:
				case WM_SYSKEYDOWN:
					//bool down = true;
					//case WM_KEYUP:
					//case WM_SYSKEYUP:
					//is_repeat = lParam & (1 << 26);
					//is_keyup  = lParam & (1 << 30);

					if (wParam == 'Q') {
						DestroyWindow(hWnd);
					}
					//if (wParam == VK_ESCAPE) {
					//	DestroyWindow(hWnd);
					//}
					else if (wParam == VK_RETURN) {
						g_view->text_editor_mode = text_editor_mode_edit;
						g_display.need_redraw_not = 0;
					}

					//else if (wParam == VK_ESCAPE) {
					//	MessageBoxA(0, (char*)"VK_ESCAPE", (char*)"eh", 0);
					//	g_display.need_redraw_not = 0;
					//}

					//else if (wParam == 'F') {
					//	diskoffset();
					//	g_display.need_redraw_not = 0;
					//}

					//else if (wParam == 'U') {
					//	if (g_background_color == 0x00000000) {
					//		g_background_color = 0x0000FFFF;
					//	}
					//	else {
					//		g_background_color = 0x00000000;
					//	}
					//	g_display.need_redraw_not = 0;
					//}

					else if (wParam == VK_OEM_2) { // '/'
						search_mode = true;
						search_string_used = 0;
						g_display.need_redraw_not = 0;
					}

					else if (wParam == 'T') {
						if (GetKeyState(VK_CONTROL) & 0x8000 && GetKeyState(VK_SHIFT) & 0x8000) {
							text_tabify_and_fix_newlines();
						}
					}

					else if (wParam == '4') {

						strcpy_s(filepath_buf, filepath_buf_maxlen, "c:/shared/text/RANDOM.txt");
						textfile_open();

					}

					else if (wParam == 'P') {

						strcpy_s(filepath_buf, filepath_buf_maxlen, "c:/shared/my_programs/firefox_addons/SaveVIDEOS@homework/content/SaveVIDEOS.js");
						textfile_open();

					}

					else if (wParam == 'A') {
						text_select_func();
					}

					else if (wParam == VK_OEM_1) { //';'

						if (GetKeyState(VK_SHIFT) & 0x8000) {
							text_copy_string(text->path);
						}
						else {

							int cursor_backup = g_view->cursor;

							text_select_line();

							int size = g_view->selection - g_view->cursor;
							if (size > filepath_buf_maxlen) size = filepath_buf_maxlen;

							char* from = text->textfilebuf + g_view->cursor;
							if (size >= filepath_buf_maxlen) size = filepath_buf_maxlen - 1;
							memcpy(filepath_buf, from, size);
							//memcpy_s(filepath_buf, sizeof(filepath_buf), from, size);
							*(filepath_buf + size) = 0;

							g_view->cursor = cursor_backup;

							textfile_open();
						}

					}

					else if (wParam == VK_LEFT || wParam == 'J') {
						text_left();

						break;
					}
					else if (wParam == VK_RIGHT || wParam == 'L') {
						text_right();

						break;
					}

					else if (wParam == VK_UP || wParam == 'I') {
						text_up_where(1);

					}

					else if (wParam == VK_DOWN || wParam == 'K') {
						text_down_where(1);

					}
					else if (wParam == 'M') {
						if (GetKeyState(VK_SHIFT) & 0x8000) {
							text_scroll_down_where(3);
							text_down_where(3);
						}
						else {
							text_down_where(3);
						}

					}
					else if (wParam == 'O') {
						if (GetKeyState(VK_SHIFT) & 0x8000) {
							text_scroll_up_where(3);
							text_up_where(3);
						}
						else {
							text_up_where(3);
						}
					}

					else if (wParam == VK_PRIOR) {
						g_view->screen_start = 0;
						g_display.need_redraw_not = 0;
					}

					else if (wParam == 'S') {
						g_view->selection = g_view->cursor;
						g_view->text_editor_mode = text_editor_mode_select;
						g_display.need_redraw_not = 0;
					}

					else if (wParam == 'Z') {
						if (GetKeyState(VK_SHIFT) & 0x8000) {
							undo_redo();
						}
						else {
							undo();
						}
						break;
					}

					else if (wParam == 'V') {
						text_copy_paste(hWnd);
					}

					else if (wParam == 'H') {
						g_display.need_redraw_not = false;
						if (GetKeyState(VK_SHIFT) & 0x8000) {
							//int oldcur = text->cursor;
							char* cur = text->textfilebuf + g_view->cursor;
							char c = *cur;
							if (cur <= text->textfilebuf) {
								//text_insert_char('\n');
								text_insert_newline_and_tabs();
								cur--;
								g_view->cursor--;
							}
							else {
								cur--;
								g_view->cursor--;
								while (true) {
									if (cur <= text->textfilebuf) break;
									char c = *cur;
									if (c == '\n') break;
									cur--;
									g_view->cursor--;
								}

								//text_insert_char('\n');
								text_insert_newline_and_tabs();
							}
							//text->cursor = oldcur + 1;
						}
						else {
							char* cur = text->textfilebuf + g_view->cursor;
							char* cur_end = text->textfilebuf + text->textfilesize;
							while (true) {
								if (cur == cur_end) break;
								char c = *cur;
								if (c == 0 || c == '\n') break;
								cur++;
								g_view->cursor++;
							}
							//text_insert_char('\n');
							text_insert_newline_and_tabs();
						}
						undo_commit();

						text_scroll_sanitize();

					}

					else if (wParam == 'D') {
						g_display.need_redraw_not = false;
						g_view->text_editor_mode = text_editor_mode_select;

						//if (!GetKeyState(VK_SHIFT) & 0x8000) {
						//	text_up_where(1);
						//}

						text_select_line();

						text_delete_selection();

						if (GetKeyState(VK_SHIFT) & 0x8000) {
							text_up_where(1);
						}

						g_view->text_editor_mode = text_editor_mode_navigation;

						undo_commit();

					}

					return 0;

					//case WM_RBUTTONUP:
					//	DestroyWindow(hWnd);
					//	return 0;
				}

			}
			else if (g_view->text_editor_mode == text_editor_mode_select) { //selection mode

				switch (msg)
				{
				case WM_KEYDOWN:
				case WM_SYSKEYDOWN:
					//down = true;
					//case WM_KEYUP:
					//case WM_SYSKEYUP:
					//is_repeat = lParam & (1 << 26);
					//is_keyup  = lParam & (1 << 30);

					if (wParam == VK_ESCAPE) {
						g_view->text_editor_mode = text_editor_mode_navigation;
						g_display.need_redraw_not = 0;
					}

					else if (wParam == VK_UP || wParam == 'I') {
						text_up_where(1);

					}

					else if (wParam == VK_DOWN || wParam == 'K') {
						text_down_where(1);

					}
					else if (wParam == VK_LEFT || wParam == 'J') {
						text_left();

						break;
					}
					else if (wParam == VK_RIGHT || wParam == 'L') {
						text_right();

						break;
					}
					else if (wParam == 'M') {
						if (!(GetKeyState(VK_SHIFT) & 0x8000)) {
							text_down_where(3);
						}
						text_scroll_down_where(3);
					}
					else if (wParam == 'O') {
						if (!(GetKeyState(VK_SHIFT) & 0x8000)) {
							text_up_where(3);
						}
						text_scroll_up_where(3);
					}

					else if (wParam == 'D') {
						text_delete_selection();
						g_view->text_editor_mode = text_editor_mode_navigation;
					}

					else if (wParam == VK_RETURN) {
						text_delete_selection();
						g_view->text_editor_mode = text_editor_mode_edit;
					}

					else if (wParam == 'X') {
						//cut

						text_copy_cut(hWnd);



					}

					else if (wParam == 'C') {
						//copy

						text_copy(hWnd);
					}

					else if (wParam == 'V') {
						text_delete_selection();
						text_copy_paste(hWnd);

						g_view->text_editor_mode = text_editor_mode_navigation;

					}

					else if (wParam == VK_OEM_2) { //'/' //'?'
												   /*text_select_line();*/
												   /*goto label_dot;*/
					}

					return 0;
				}
			}

		}




	}
}

