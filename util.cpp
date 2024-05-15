#include "pch.h"
#include "util.h"

//used by:
//int_to_wstring
wchar_t g_tempzero[12];
wchar_t* g_tempzero_endzero = &g_tempzero[10]; //[10] and [11] will always be 0, for wchar_t and for char.
char* g_tempzero_hexstart = (char*)g_tempzero_endzero - 8;





wchar* WString::c_str() {
	return this->warr;
}


WString& operator<<(WString& a, wchar_t const* src) {
	wchar* dst = (wchar*)(a.arr + a.len);
	while (true) {
		wchar temp = *src++;
		if (temp == 0) break;

		*dst++ = temp;
		a.len += 2; //LATER: just get the length by comparing pointers.
		if (((addr)dst & 0xff) == 0) break;
	}
	*dst = 0;
	return a;
}

WString& operator<<(WString& a, int num) {
	if (a.len > 0xf0) return a; //LATER: figure out exactly the max length a number can take.
	wchar* dst = (wchar*)(a.arr + a.len - 2);
	if (num == 0) {
		*dst++ = L'0';
		a.len += 2;
		return a;
	}

	uint alen = 1;
	{
		uint temp = num;
		while (true) {
			temp = temp / 10;
			if (temp == 0) break;
			alen++;
		}
	}
	dst++;
	*dst = 0;
	dst--;
	dst += alen;
	a.len += alen * 2;

	while (num) {
		uint reminder = num % 10;
		num = num / 10;
		*dst-- = L'0' + reminder;
	}
	return a;
}



u1 error_buffer[0x100]; //LATER: do something about this maybe

WString* WString::New() {
	return (WString*)error_buffer;
}

wchar_t* wcscpy_return_end(wchar_t* dst, const wchar_t* src) {
	while (true) {
		wchar_t temp = *src++;
		*dst++ = temp;
		if (!temp) return dst - 1;
	}
};

char* strcpy_return_end(char* dst, char* src) {
	while (true) {
		char temp = *src++;
		*dst++ = temp;
		if (!temp) return dst - 1;
	}
};


wchar_t* int_to_wstring(int the_int) {
	//if (the_int > 9999999999)
	wchar_t* cur = g_tempzero_endzero;
	cur--;
	if (the_int == 0) {
		*cur = L'0';
		return cur;
	}
	int ostatok;
	while (the_int) {
		ostatok = the_int % 10;
		the_int = the_int / 10;

		*cur = L'0' + ostatok;
		cur--;
	}
	cur++;

	return cur;
}

char* int_to_string(int the_int) {
	//if (the_int > 9999999999)
	char* cur = (char*)g_tempzero_endzero;
	cur--;
	if (the_int == 0) {
		*cur = '0';
		return cur;
	}
	int ostatok;
	while (the_int) {
		ostatok = the_int % 10;
		the_int = the_int / 10;

		*cur = '0' + ostatok;
		cur--;
	}
	cur++;

	return cur;
}

char* int_to_hexstring(UINT32 the_int) {
	//if (the_int > 9999999999)
	char* cur = (char*)g_tempzero_endzero;
	cur--;
	int ostatok;
	while (the_int) {
		ostatok = the_int % 16;
		the_int = the_int / 16;

		if (ostatok < 10) {
			*cur = '0' + ostatok;
		}
		else {
			*cur = 'a' + ostatok - 10;
		}
		cur--;
	}
	while (cur != g_tempzero_hexstart) {
		*cur-- = ' ';
	}
	cur++;

	return cur;
}

int wstring_to_int(wchar_t* the_string) {
	wchar_t* cur = the_string;
	int the_int = 0;
	int x_num = 1;

	//find the zero
	while (*cur != 0) {
		cur++;
	}
	cur--;
	while (cur >= the_string) {
		the_int += (*cur - L'0') * x_num;
		x_num *= 10;
		cur--;
	}
	return the_int;
}


void THROW() {
	DWORD error = GetLastError();

	wchar* errorString;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPWSTR)&errorString,
		0, NULL);

	WString* s = WString::New();
	*s << L"THROW" << error << L" | " << errorString;
	MessageBoxW(0, s->warr, L"!!!!", 0);

	LocalFree(errorString);

	__debugbreak();

	ExitProcess(0);
}

//void THROW() {
//	//DWORD error = GetLastError();
//
//	//wchar_t* errorString;
//	//FormatMessageW(
//	//	FORMAT_MESSAGE_ALLOCATE_BUFFER |
//	//	FORMAT_MESSAGE_FROM_SYSTEM |
//	//	FORMAT_MESSAGE_IGNORE_INSERTS,
//	//	NULL,
//	//	error,
//	//	MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
//	//	(LPWSTR)&errorString,
//	//	0, NULL);
//
//	//wchar_t* numstring = int_to_wstring(error);
//	//int numstring_length = (wchar_t*)g_tempzero_endzero - numstring;
//
//
//
//	//wchar_t* buf = (wchar_t*)LocalAlloc(LMEM_ZEROINIT,
//	//	(lstrlenW(errorString) + numstring_length + 1 + 80) * sizeof(wchar_t));
//	////+ 80 is for those " | " extras down there
//
//	//wchar_t* buf2 = wcscpy_return_end(buf, L"THROW | ");
//	//wchar_t* buf3 = wcscpy_return_end(buf2, numstring);
//	//wchar_t* buf4 = wcscpy_return_end(buf3, L" | ");
//	//wcscpy(buf4, errorString);
//
//	////StringCchPrintf((LPTSTR)lpDisplayBuf, 
// ////       LocalSize(lpDisplayBuf) / sizeof(TCHAR),
//	////	TEXT("THROW: %d: %s"), 
// ////       error, errorString); 
//
//	//MessageBoxW(0, buf, L"!!!!", 0);
//
//	//LocalFree(buf);
//	//LocalFree(errorString);
//
//	__debugbreak();
//
//}

void* g_alloc_base; //not sure why exactly i keep this around...
void* g_alloc_next;
DWORD* g_alloc_free;

//stack
//dword prev

//void* g_alloc_border;
//void* g_alloc_gaps = 0; //array

//used by memget100
void* g_alloc_next100;

void THROW(char const* messagebox) {
	MessageBoxA(0, messagebox, "THROWcom", 0);
	__debugbreak();
	ExitProcess(0);
}





void* memget_noreuse() {
	THROW("don't use this");
	void* res;
	DWORD free = (DWORD)g_alloc_next;
	g_alloc_next = (char*)g_alloc_next + 0x1000;
	res = VirtualAlloc(
		(void*)free,
		0x10000,
		MEM_COMMIT,
		PAGE_READWRITE);

	//if (paranoid
	if (res == 0) THROW("memget_noreuse");

	return res;
	return nullptr;
}

void* memget() {
	return memNotSimple->get1000();
}

void memget_guard(const char* string) {
	//void* p = memget();
	//strcpy((char*)p, string);
	//DWORD oldProtect;
	//VirtualProtect(p, 0x1000, PAGE_NOACCESS, &oldProtect);

	//// https://docs.microsoft.com/en-us/windows/desktop/Memory/creating-guard-pages

	////VirtualAlloc(NULL, dwPageSize,
 ////                        MEM_RESERVE | MEM_COMMIT,
 ////                        PAGE_READONLY | PAGE_GUARD);
}

void memfree(void* in) {
	memNotSimple->free1000((char*)in);
	//VirtualFree(in, 0x1000, MEM_DECOMMIT);

	//*g_alloc_free++ = (DWORD)in;
	//if (((intptr_t)g_alloc_free & 0xFFF) == 0) {
	//	__debugbreak(); //just to notify me, I need to check this works.
	//	//g_alloc_free = g_alloc_free; //wouldn't substract 0x1000.
	//	//hm, i can't call memget inside memfree, right?
	//	//I'll just reuse this exact chunk instead, lol.
	//	DWORD* next = (DWORD*)in;
	//	//VirtualAlloc(
 // //               next,
 // //               0x1000,
 // //               MEM_COMMIT,
 // //               PAGE_READWRITE);
	//	*next++ = (DWORD)g_alloc_free;
	//	g_alloc_free = next;

	//}

}

void* memget100() {
	return memNotSimple->get100();
	//void* res = g_alloc_next100;

	//g_alloc_next100 = (char*)g_alloc_next100 + 256;

	//if (((intptr_t)g_alloc_next100 & 0xFFF) == 0) {
	//	g_alloc_next100 = memget();
	//}

	//return res;
}



void Alert(wchar const* message) {
	MessageBoxW(0, message, L"Alert()", 0);
}

void Alert(char const* message) {
	MessageBoxA(0, message, "Alert()", 0);
}


wchar_t utf8_to_wchar(char*& c) {
	wchar_t t;
	if ((*c & 0x80) == 0x00) {
		t = *c;
		++c;
	}
	else if ((*c & 0xE0) == 0xC0) {
		uint8_t t1 = *c;
		uint8_t t0 = *(c + 1);
		t0 = t0 & 0x3f;
		t1 = t1 & 0x1f;
		t = t1 << 6 | t0;
		c += 2;
	}
	else if ((*c & 0xF0) == 0xE0) {
		uint8_t t2 = *c;
		uint8_t t1 = *(c + 1);
		uint8_t t0 = *(c + 2);
		t0 = t0 & 0x3f;
		t1 = t1 & 0x3f;
		t2 = t2 & 0x0f;
		t = t2 << 12 | t1 << 6 | t0;

		c += 3;
	}
	else {
		//THROW();
		t = *c;
		++c;
	}
	return t;
}

void wchar_to_utf8(wchar_t i, u1*& c) {
	if (i <= 0x7F) {
		uint8_t t = i & 0x7F;
		*c++ = t;
		return;
	}
	else if (i <= 0x7FF) {
		uint8_t t0 = i & 0x3F;
		t0 |= 0x80;
		i >>= 6;
		uint8_t t1 = i;
		t1 |= 0xC0;

		*c++ = t1;
		*c++ = t0;

		return;
	}
	else if (i <= 0xFFFF) {
		if (i >= 0xd800 && i <= 0xdbff) THROW(); //LATER.
		uint8_t t0 = i & 0x3F;
		t0 |= 0x80;
		i >>= 6;
		uint8_t t1 = i & 0x3F;
		t1 |= 0x80;
		i >>= 6;
		uint8_t t2 = i & 0x0F;
		t2 |= 0xE0;

		*c++ = t2;
		*c++ = t1;
		*c++ = t0;
		return;
	}
	else {
		//paranoid
		THROW();
	}
}

void wchar_to_utf8(wchar_t const* from, u1*& to) {
	wchar_t* l_from = (wchar_t*)from;
	while (true) {
		wchar_to_utf8(*l_from++, to);
		if (*l_from == 0) break;
	}
}

void string_copy(char const* from, u1*& to) {
	//char* l_from = (char*)from;
	while (true) {
		*to++ = *from++;
		if (*from == 0) break;
	}
}

void wstring_copy(wchar_t const* from, wchar_t*& to) {
	//char* l_from = (char*)from;
	while (true) {
		*to++ = *from++;
		if (*from == 0) break;
	}
}

//LATER: rewrite it
unsigned int next_power_of_2(unsigned int in) {
	int out = 1;
	while (out < in) {
		out *= 2;
	}
	return out;
}


Memory* Memory::New(u4 intended_base, u4 size) {
	if (paranoid) {
		SYSTEM_INFO sSysInfo;
		GetSystemInfo(&sSysInfo);

		if (sSysInfo.dwPageSize != 0x1000) {
			THROW();
		}

		if (sSysInfo.dwAllocationGranularity != 0x10000) {
			THROW();
		}
	}


	char* base = (char*)VirtualAlloc(
		(void*)intended_base,
		size,
		MEM_RESERVE,
		PAGE_NOACCESS);

	if (paranoid) {
		if (!base) {
			THROW("VirtualAlloc failed");
		}
		if ((u4)base != intended_base) {
			THROW("Different base");
		}
	}


	VirtualAlloc(
		(void*)base,
		howmuchtocommit,
		MEM_COMMIT,
		PAGE_READWRITE);

	auto mem = (Memory*)base;

	mem->signature = 'haha';
	mem->base = u4(base);
	mem->last = (u1*)(mem + 1);
	mem->allocated = ((u1*)base) + howmuchtocommit;
	mem->allocated = mem->allocated - allocated_shift;
	mem->cap = size;
	return mem;
}


void Memory::NewOutside(u4 intended_base, u4 size) {
	if (paranoid) {
		SYSTEM_INFO sSysInfo;
		GetSystemInfo(&sSysInfo);

		if (sSysInfo.dwPageSize != 0x1000) {
			THROW();
		}

		if (sSysInfo.dwAllocationGranularity != 0x10000) {
			THROW();
		}
	}


try_again:

	char* base = (char*)VirtualAlloc(
		(void*)intended_base,
		size,
		MEM_RESERVE,
		PAGE_NOACCESS);

	//LATER: add this for other New
	if (base == 0) {
		intended_base += size;
		if (intended_base >= 0x70000000) THROW("VirtualAlloc really failed");
		goto try_again;
	}

	if (paranoid) {
		if (!base) {
			THROW("VirtualAlloc failed");
		}
		if ((u4)base != intended_base) {
			THROW("Different base");
		}
	}


	VirtualAlloc(
		(void*)base,
		howmuchtocommit,
		MEM_COMMIT,
		PAGE_READWRITE);

	auto mem = this;

	mem->signature = 'haha';
	mem->base = u4(base);
	mem->last = (u1*)u4(base);
	mem->allocated = ((u1*)base) + howmuchtocommit;
	mem->allocated = mem->allocated - allocated_shift;
	mem->cap = size;
	return;
}

void Memory::Grow() {
	if (!(last > allocated)) {
		return;
	}
	u1* l_allocated = allocated;
	do {
		l_allocated += howmuchtocommit; //LATER: do rounding magic later
	} while (last > l_allocated);
	u4 commit = l_allocated - allocated;
	void* why = (void*)(allocated + allocated_shift);
	LPVOID res = VirtualAlloc(
		(void*)(allocated + allocated_shift),
		commit,
		MEM_COMMIT,
		PAGE_READWRITE);
	allocated = l_allocated;
}

void* Memory::Get(size_t sz) {
	void* res = this->last;
	this->last = this->last + sz;
	if (last > allocated) {
		Grow();
	}
	return res;
}

void* Memory::Get1000() {
	return Get(0x1000);
}

void Memory::EnsureCapacity(u4 l_size) {
	u4 current_allocated = u4(this->last) - this->base;
	if (l_size > current_allocated) {
		this->Get(l_size - current_allocated);
	}
}

void Memory::MakeSureItHasAtLeastThisMuchFreeSpace(u4 l_size, u1* writecur) {
	u4 current_allocated = u4(this->last) - this->base;
	u4 how_much_we_need = writecur - bytep(this->base) + l_size;
	if (how_much_we_need > current_allocated) {
		this->Get(next_power_of_2(how_much_we_need) - current_allocated);
	}
}

void Memory::FileLoad(wchar_t const* filepath, u4* out_filesize) {
	if (paranoid && intptr_t(this) == intptr_t(this->base)) THROW("use it with plain files only for now");
	DWORD desiredAccess;
	DWORD shareMode;
	DWORD creationDistribution;
	desiredAccess = GENERIC_READ;
	shareMode = FILE_SHARE_READ;
	creationDistribution = OPEN_EXISTING;
	//DWORD flagsAndAttributes = FILE_FLAG_NO_BUFFERING;
	DWORD flagsAndAttributes = 0; //I have no idea if FILE_FLAG_NO_BUFFERING makes anything faster or slower.
	HANDLE file = CreateFileW(filepath, desiredAccess, shareMode, NULL, creationDistribution, flagsAndAttributes, NULL);
	if (file == INVALID_HANDLE_VALUE) THROW();
	BY_HANDLE_FILE_INFORMATION fileInformation;
	GetFileInformationByHandle(file, &fileInformation);
	u4 filesize = fileInformation.nFileSizeLow;

	static u4 sector_size = 0; //LATER: maybe move it to a global
	if (!sector_size) {
		DWORD sectorsPerCluster;
		DWORD bytesPerSector;
		DWORD numberOfFreeClusters;
		DWORD totalNumberOfClusters;
		GetDiskFreeSpaceW(NULL, &sectorsPerCluster, &bytesPerSector, &numberOfFreeClusters, &totalNumberOfClusters);
		sector_size = bytesPerSector;
	}

	u4 filesize_aligned;
	if (filesize < sector_size) {
		filesize_aligned = sector_size;
	}
	else {
		filesize_aligned = next_power_of_2(filesize);
	}

	if (filesize >= u4(this->last) - this->base) {
		this->EnsureCapacity(filesize_aligned);
	}

	DWORD numberOfBytesRead;
	BOOL res = ReadFile(file, LPVOID(this->base), filesize_aligned, &numberOfBytesRead, NULL);
	if (res == 0) THROW();

	CloseHandle(file);

	*out_filesize = numberOfBytesRead;
}

void Memory::FileWrite(wchar_t const* filepath, u4 filesize) {
	DWORD desiredAccess;
	DWORD shareMode;
	DWORD creationDistribution;
	desiredAccess = GENERIC_WRITE;
	shareMode = 0;
	creationDistribution = OPEN_ALWAYS;
	HANDLE file = CreateFileW(filepath, desiredAccess, shareMode, NULL, creationDistribution, 0, NULL);
	if (file == INVALID_HANDLE_VALUE) THROW();

	DWORD numberOfBytesWritten;
	BOOL res = WriteFile(file, LPVOID(this->base), filesize, &numberOfBytesWritten, NULL);
	if (res == 0) THROW();

	SetEndOfFile(file); //trimming file

	CloseHandle(file);

}


MemoryNotSimple* MemoryNotSimple::Init() {
	//this->intended_base = intended_base;
	//this->size = size;

	if (paranoid) {
		SYSTEM_INFO sSysInfo;
		GetSystemInfo(&sSysInfo);

		if (sSysInfo.dwPageSize != 0x1000) {
			THROW();
		}

		if (sSysInfo.dwAllocationGranularity != 0x10000) {
			THROW();
		}
	}

	char* res = (char*)VirtualAlloc(
		(void*)intended_base,
		size,
		MEM_RESERVE,
		PAGE_NOACCESS);

	if (paranoid && !res) {
		THROW("VirtualAlloc failed");
		//I'm setting memory address manually, because I want it to fit in dword.
		//Don't care about address randomization etc for now.
	}

	//g_alloc_next = m_base;
	//g_alloc_free = (DWORD*)memget_noreuse() + 1;
	//memget_guard("g_alloc_free");

	//g_alloc_border = (char*) g_alloc_base + 4096*256;

	//g_alloc_next100 = memget();

	VirtualAlloc(
		(void*)res,
		0x10000,
		MEM_COMMIT,
		PAGE_READWRITE);

	auto mem = (MemoryNotSimple*)res;
	mem->last100 = res + 0x1000;
	mem->last1000 = res + 0x2000;
	return mem;
}



char* MemoryNotSimple::get1000() {
	if ((((addr)last1000) & 0xFFFF) == 0) {
		if (paranoid && last1000 >= (char*)intended_base + size) {
			THROW("MemLists::get1000, out of memory");
		}
		VirtualAlloc(
			(void*)last1000,
			0x10000,
			MEM_COMMIT,
			PAGE_READWRITE);
	}

	char* res = last1000;
	last1000 += 0x1000;
	return res;
}

char* MemoryNotSimple::get100() {
	//if (!m_free100.isEmpty()) {
		//char* free = m_free100.delFirst();
	//}

	//this->m_free100

	if ((((addr)last100) & 0xFFF) == 0) {
		last100 = get1000();
	}

	char* res = last100;
	last100 += 0x100;
	return res;
}

void MemoryNotSimple::free100(char* in) {
	//TODO
}

void MemoryNotSimple::free1000(char* in) {
	//TODO
}

//LATER: I need to get memory allocator from address itself instead of relying on any globals
//void* operator new(size_t sz) {
//	return mem->Get(sz);
//}


void* operator new(size_t sz, Memory* l_mem) {
	return l_mem->Get(sz);
}

void THROWLAZY() {
	MessageBoxW(0, L"THROWLAZY()", 0, 0);
	__debugbreak();
	ExitProcess(0);
}

//MemoryPlain* MemoryPlain::New(u4 intended_base, u4 size)
//{
//	
//	return nullptr;
//}


Memory* GetMemoryRoot(u4 address) {

	//if (address > 0x50000000) {}
	// 0x10 00 00 00
	//  0xF FF FF FF
	address = address & !0xFFFFFFF;
	return (Memory*)address;
}
