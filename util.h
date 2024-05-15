#pragma once

#include "pch.h"

#include <stdint.h>

//typedef signed char        int8_t;
//typedef short              int16_t;
//typedef int                int32_t;
//typedef long long          int64_t;
//typedef unsigned char      uint8_t;
//typedef unsigned short     uint16_t;
//typedef unsigned int       uint32_t;
//typedef unsigned long long uint64_t;

using i1 = int8_t;
using i2 = int16_t;
using i4 = int32_t;
using i8 = int64_t;
using u1 = uint8_t;
using u2 = uint16_t;
using u4 = uint32_t;
using u8 = uint64_t;

using f4 = float;
using f8 = double;

using wchar = wchar_t;

using addr = intptr_t; //I use this when I need to access address of a pointer as a number.

using uint = unsigned int;
using bytep = u1*;


inline constexpr bool paranoid = true;
inline constexpr bool paranoid2 = false;

class Memory;

//inline Memory* mem;

unsigned int next_power_of_2(unsigned int in);

// A selfrelative pointer. Stole code from a video of one valve employee.
template <typename T>
struct RP
{
	i4 offset;

	__forceinline T* operator->()
	{
		//Assert(m_nOffset != 0);
		return (T*)((char*)&offset + offset);
	}

	__forceinline void operator = (T* p) {
		//m_nOffset = p ? ((byte*)p) - (byte*)t : 0;
		offset = i4(bytep(p) - bytep(&offset));
	}

	__forceinline bool operator == (RP<T> p) {
		return this->offset == p.offset;
	}

	__forceinline bool isEmpty() {
		return offset == 0;
	}

	__forceinline bool isNotEmpty() {
		return offset != 0;
	}

	__forceinline void makeEmpty() {
		offset = 0;
	}

	__forceinline operator T* () {
		return (T*)((char*)&offset + offset);
	}

	__forceinline explicit operator u4 () {
		return (u4)((char*)&offset + offset);
	}

	__forceinline T* Absolute() {
		return (T*)((char*)&offset + offset);
	}

};

// Absolute pointer that takes 4 bytes of space.
template <typename T>
struct P4
{
	u4 address;

	__forceinline T* operator->()
	{
		return (T*)&address;
	}

	__forceinline void operator = (T* p) {
		address = (u4)p;
	}



};


struct String {
	u1 len;
	RP<char*> ptr;
};

//This string cannot be longer than 0x80 wchars. Add will not work if it causes overflow.
struct WString {
	u2 len;
	static const int arraysize = (0x100 - 4) / 2 + 1;

	union {
		char arr[arraysize * 2];
		wchar warr[arraysize];
	};

	WString() = delete;
	static WString* New();

	wchar* c_str();

	__forceinline
		void Alert() {
		MessageBoxW(0, warr, L"MessageBox", 0);
	}

};

WString& operator<<(WString& a, wchar_t const* src);
WString& operator<<(WString& a, int num);




//VAR

extern wchar_t g_tempzero[12];
extern wchar_t* g_tempzero_endzero;
extern char* g_tempzero_hexstart;

//FUNCAHEAD

char* int_to_string(int the_int);
wchar_t* int_to_wstring(int the_int);
wchar_t* wcscpy_return_end(wchar_t* dst, const wchar_t* src);
char* int_to_hexstring(UINT32 the_int);


extern void* g_alloc_next;

void* memget();
void memfree(void* in);
void mem_init();
void memget_guard(const char* string);

void THROW();
void THROW(char const* messagebox);

class MemoryNotSimple;
template <typename T>
class List;

class MemoryNotSimple {
public:

	static constexpr void* intended_base = (void*)0x30000000;
	static constexpr int size = 0x10000000;
	//u4 intended_base;
	//u4 size;
	u4 signature;
	char* base;
	char* last1000;
	char* last100;

	List<u4*>* freeChunks100;
	List<u4*>* freeChunks1000;

	MemoryNotSimple* Init();

	char* get100();

	char* get1000();

	void free100(char* freethis);

	void free1000(char* freethis);
};

template <typename T>
class List {
public:
	//static constexpr 
	RP<List<T>> next;
	RP<List<T>> prev;
	u4 used;
	u4 cap;
	static constexpr int capconst = (0x1000 - 4 * 4) / sizeof(T);
	T arr[capconst];


	List() = delete;
	static List* New(Memory* l_mem);

	List<T>* addLast(T* addthis);
	List<T>* addLast(T addthis);
	void reserveLast(List<T>*& list, T*& addthis);
	void Next(List<T>*& list, T*& fromthis, T*& fromthis_end);
	void NextInit(T*& fromthis, T*& fromthis_end);

	void Grow();

	i4 IndexOf(T search);
	i4 IndexOfP(T* search); //LATER: delme
	T* Get(u4 index);
	u4 Count();
	void AddIfNotAlreadyIn(T addthis);
};


//class MemoryPlain {
//public:
//	static constexpr void* intended_base = (void*)0x55000000;
//	static constexpr int size = 0x1000000;
//	u1* base;
//	u1* last1000;
//	u4 cap;
//	u4 used;
//
//
//	MemoryPlain() = delete;
//	static MemoryPlain* New(u4 intended_base, u4 size);
//
//	//void Init();
//	char* Get1000();
//	void Reset();
//};




inline MemoryNotSimple* memNotSimple; //LATER: delete

//inline MemoryPlain* memTemp;


struct Folder {
	List<WString*>* files;

	Folder() = delete;
	Folder* New(wchar*);
	Folder* New(WString*);
};


class Memory {
public:
	u4 signature; //something like 'sign', 4 char string
	u4 base;
	RP<u1> last;
	RP<u1> allocated;
	u4 cap;

	//LATER: this doesn't check for cap overflow.
	static const size_t howmuchtocommit = 0x100000;
	static const size_t allocated_shift = 0x10000; //this isn't zero to allow some Lists to grow a bit beyond where they are normally allowed. //SCARY
	//^ this is ignored for Memory that was initiated with NewOutside

	//Memory() = delete;

	static Memory* New(u4 intended_base, u4 size);
	void NewOutside(u4 intended_base, u4 size);

	void Grow();

	void* Get(size_t sz);
	void* Get1000();
	void EnsureCapacity(u4 l_size);
	void MakeSureItHasAtLeastThisMuchFreeSpace(u4 l_size, u1* writecur);


	void FileLoad(wchar_t const* filepath, u4* out_filesize);
	void FileWrite(wchar_t const* filepath, u4 filesize);
};



template <typename T>
List<T>* List<T>::New(Memory* l_mem) {
	//auto nothis = this;
	//auto thisaddr = (intptr_t)&(nothis->arr);
	//auto thismem = GetMemoryRoot(u4(this));
	List<T>* res = (List<T>*)l_mem->Get1000();
	res->next.makeEmpty();
	res->prev.makeEmpty();
	res->used = 0;
	res->cap = 0x1000 / sizeof(T);
	//(u4*)(res->arr) = 0;
	//(u4*)(res->arr + 1) = 0;

	return res;
}

template <typename T> __forceinline
List<T>* List<T>::addLast(T* addthis) {
	this->arr[this->used] = addthis;
	this->used += 1;

	if (this->used >= this->cap) {
		this->Grow();
		return this->next;
	}
	return this;
}

template <typename T> __forceinline
List<T>* List<T>::addLast(T addthis) {
	this->arr[this->used] = addthis;
	this->used += 1;

	if (this->used >= this->cap) {
		this->Grow();
		return this->next;
	}
	return this;
}

template <typename T> __forceinline
void List<T>::reserveLast(List<T>*& list, T*& returnthis) {
	returnthis = &list->arr[list->used];
	list->used += 1;

	if (list->used >= list->cap) {
		list->Grow();
		list = list->next;
	}
}

template <typename T> __forceinline
void List<T>::NextInit(T*& fromthis, T*& fromthis_end) {
	fromthis_end = &this->arr[this->used];
}

template <typename T> __forceinline
void List<T>::Next(List<T>*& list, T*& fromthis, T*& fromthis_end) {
	fromthis++;
	if (fromthis != fromthis_end) {
		return;
	}
	if (list->next.isEmpty()) {
		fromthis = nullptr;
		return;
	}
	list = list->next;
	fromthis = &list->arr[0];
	fromthis_end = &list->arr[list->used];
	return;
}

template <typename T>
void List<T>::AddIfNotAlreadyIn(T addthis) {
	auto list = this;
	bool found = false;
	if (list->used) {
		T* cur = list->arr;
		T* cur_end = cur + list->used;

		while (true) {

			if (*cur == addthis) {
				return;
			}

			cur++;

			if (cur != cur_end) continue;

			if (list->next.isEmpty()) {
				break;
			}
			list = list->next;
			if (list->used == 0) break;
			cur = list->arr;
			cur_end = cur + list->used;
		}
	}

	list->addLast(addthis);
}

Memory* GetMemoryRoot(u4 address);

template <typename T>
void List<T>::Grow() {
	this->next = List<T>::New(GetMemoryRoot(u4(this)));
	this->next->prev = this;
}

template <typename T>
i4 List<T>::IndexOf(T search) {
	auto list = this;
	i4 res = 0;
	if (list->used) {
		T* cur = list->arr;
		T* cur_first = cur;
		T* cur_end = cur + list->used;

		while (true) {

			if (*cur == search) return res + (cur - cur_first);

			cur++;

			if (cur != cur_end) continue;

			if (list->next.isEmpty()) {
				break;
			}
			res += list->used;
			list = list->next;
			cur = list->arr;
			cur_first = cur;
			cur_end = cur + list->used;
		}
	}
	return -1;
}

//I only use this for debugging, delete it later
template <typename T> __forceinline
i4 List<T>::IndexOfP(T* search) {
	auto list = this;
	i4 res = 0;
	while (true) {
		if (list->arr <= search && search <= &list->arr[list->used]) {
			res += search - list->arr;
			return res;
		}
		if (list->next.isEmpty()) return -1;
		res += list->used;
		list = list->next;
	}
}

template <typename T> __forceinline
T* List<T>::Get(u4 index) {
	auto list = this;
	while (index >= list->used) {
		if (list->next.isEmpty()) return nullptr;
		index -= list->used;
		list = list->next;
	}
	return &list->arr[index];
}

template <typename T> __forceinline
u4 List<T>::Count() {
	auto list = this;
	u4 res = list->used;
	while (list->next.isNotEmpty()) {
		list = list->next;
		res += list->used;
	}
	return res;
}

void Alert(wchar const* message);
void Alert(char const* message);

wchar_t utf8_to_wchar(char*& c);
void wchar_to_utf8(wchar_t i, u1*& c);
void wchar_to_utf8(wchar_t const* from, u1*& to);
void string_copy(char const* from, u1*& to);
void wstring_copy(wchar_t const* from, wchar_t*& to);



void* operator new(size_t sz);
void* operator new(size_t sz, Memory* mem);


class File {
public:
	HANDLE handle;

	File() = delete;
	File(char mode, wchar_t* filepath) {
		//LATER: maybe a good place for template
		//LATER: consider FILE_FLAG_NO_BUFFERING and others
		DWORD desiredAccess;
		DWORD shareMode;
		DWORD creationDistribution;
		if (mode == 'r') {
			desiredAccess = GENERIC_READ;
			shareMode = FILE_SHARE_READ;
			creationDistribution = OPEN_EXISTING;
		}
		else if (mode == 'w') {
			desiredAccess = GENERIC_WRITE;
			shareMode = 0;
			creationDistribution = OPEN_ALWAYS;
		}
		handle = CreateFileW(filepath, desiredAccess, shareMode, NULL, creationDistribution, 0, NULL);
	}

	bool Exists() {
		return handle != 0;
	}
};

void THROWLAZY();