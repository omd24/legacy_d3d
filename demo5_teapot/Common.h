#pragma once

#pragma warning (disable: 6011)     // dereferencing a potentially null pointer
#pragma warning (disable: 26495)    // not initializing struct members
#pragma warning (disable: 26812)    // unscoped enum

#include <windows.h>

#include <d3d9.h>
#include <d3dx9.h>

#include <stdlib.h>
#include <tchar.h>
#include <memory.h>

#include <stdbool.h>

// _ASSERT_EXPR macro
#include <crtdbg.h>
#ifdef _DEBUG
#pragma comment(lib, "ucrtd.lib")
#endif
