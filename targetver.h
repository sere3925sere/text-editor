#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <WinSDKVer.h>

#ifdef  _WIN32_WINNT
#undef  _WIN32_WINNT
#endif

#define _WIN32_WINNT 0x0501
//windows xp
//https://stackoverflow.com/questions/41376557/win32-winnt-winver-macro-redefinition
//https://github.com/Microsoft/vs-setup-samples/blob/master/Setup.Configuration.VC/targetver.h


#include <SDKDDKVer.h>
