// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

/*
#pragma message("Build of libOpenProtocol Started on " __DATE__ " " __TIME__)
#pragma message("====================")
#pragma message("Build Configuration:")

#ifdef _DEBUG
#pragma message("Debug Mode: Yes")
#else
#pragma message("Debug Mode: No")
#endif

#ifdef _WIN64
#pragma message("Compiling for WIN64: Yes")
#else
#pragma message("Compiling for WIN64: No")
#endif

#pragma message("==============================")
#pragma message("Processing: Windows Headers")
*/

#ifndef _WIN32_WINNT		// Windows XP 以降のバージョンに固有の機能の使用を許可します。                   
#define _WIN32_WINNT 0x0501	// これを Windows の他のバージョン向けに適切な値に変更してください。
#endif

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <stack>
using namespace std;

// TODO: プログラムに必要な追加ヘッダーをここで参照してください。
#include "libOpenProtocol.h"
