#pragma once

#include "targetver.h"
//#define WIN32_LEAN_AND_MEAN             // Исключите редко используемые компоненты из заголовков Windows
//// Файлы заголовков Windows
//#include <windows.h>
//// Файлы заголовков среды выполнения C
//#include <stdlib.h>
//#include <malloc.h>
//#include <memory.h>
//#include <tchar.h>

// reference additional headers your program requires here

#define _CRT_SECURE_NO_DEPRECATE

#pragma warning( disable : 4996 ) // disable deprecated warning 
//#include <strsafe.h>
#pragma warning( default : 4996 )



#pragma warning (disable : 4010) //something about safe fopen_s

#pragma warning (disable : 4311) //pointer conversion
#pragma warning (disable : 4312)

#pragma warning (disable : 4995) //'wcscat': name was marked as #pragma deprecated

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fileapi.h>



#include <stdio.h>



#include <d3d9.h>
#pragma comment(lib,"d3d9.lib")

//#include <d3dx9.h>
//for D3DXCreateTextureFromFile





#include <windowsx.h> //for GET_X_LPARAM

#include <setjmp.h>

#include <winnt.h> //for MemoryBarrier

//HEADERS

