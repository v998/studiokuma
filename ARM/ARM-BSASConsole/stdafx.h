/*
// stdafx.h : 可在此標頭檔中包含標準的系統 Include 檔，
// 或是經常使用卻很少變更的專案專用 Include 檔案
//

#pragma once

//#define _AFX_NO_OLE_SUPPORT
#define _AFX_NO_DB_SUPPORT
#define _AFX_NO_DAO_SUPPORT
#define _AFX_NO_AFXCMN_SUPPORT

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// 從 Windows 標頭排除不常使用的成員
#endif

// 如果您有必須優先選取的平台，請修改下列定義。
// 參考 MSDN 取得不同平台對應值的最新資訊。
#ifndef WINVER				// 允許使用 Windows 95 與 Windows NT 4 (含) 以後版本的特定功能。
#define WINVER 0x0400		// 將它變更為針對 Windows 98 和 Windows 2000 (含) 以後版本適當的值。
#endif

#ifndef _WIN32_WINNT		// 允許使用 Windows NT 4 (含) 以後版本的特定功能。
#define _WIN32_WINNT 0x0500	// 將它變更為針對 Windows 2000 (含) 以後版本的適當值。
#endif						

#ifndef _WIN32_WINDOWS		// 允許使用 Windows 98 (含) 以後版本的特定功能。
#define _WIN32_WINDOWS 0x0410 // 將它變更為針對 Windows Me (含) 以後版本適當的值。
#endif

#ifndef _WIN32_IE			// 允許使用 IE 4.0 (含) 以後版本的特定功能。
#define _WIN32_IE 0x0400	// 將它變更為針對 IE 5.0 (含) 以後版本適當的值。
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// 明確定義部分的 CString 建構函式

#include <afxwin.h>         // MFC 核心與標準元件
#include <afxext.h>         // MFC 擴充功能

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE 類別
#include <afxodlgs.h>       // MFC OLE 對話方塊類別
#include <afxdisp.h>        // MFC Automation 類別
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>			// MFC ODBC 資料庫類別
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>			// MFC DAO 資料庫類別
#endif // _AFX_NO_DAO_SUPPORT

#include <afxdtctl.h>		// MFC 支援的 Internet Explorer 4 通用控制項
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC 支援的 Windows 通用控制項
#endif // _AFX_NO_AFXCMN_SUPPORT

*/

// stdafx.h : 標準ソЁЗЪу ユ⑦ヱюみЭ иャユюソユ⑦ヱюみЭ иャユю、ネギゾ
// �繴茼^�袸穧hゑ、ろコやネベ�藹鬷�ホスゆ、к①ЖラヱЬ�G用ソユ⑦ヱюみЭ иャユю
// メ記述ウネエ。

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Windows лЧФみろヘ使用イホサゆスゆ部分メ除外ウネエ。
#endif

// 下ザ指定イホギ定義ソ前ズ�f象кьЧЬиルみуメ指定ウスんホタスヘスゆ場合、以下ソ定義メ�藹鬷�サゑクイゆ。
// 異スペкьЧЬиルみуズ�f�Nエペ�怪R�kエペ最新情報ズコゆサゾ、MSDN メ�繴蚙�サゑクイゆ。
#ifndef WINVER				// Windows XP 以降ソдみЖъ⑦ズ固有ソ機能ソ使用メ許可ウネエ。
#define WINVER 0x0501		// アホメ Windows ソ他ソдみЖъ⑦向んズ適切ス�怪R�藹鬷�サゑクイゆ。
#endif

#ifndef _WIN32_WINNT		// Windows XP 以降ソдみЖъ⑦ズ固有ソ機能ソ使用メ許可ウネエ。                   
#define _WIN32_WINNT 0x0501	// アホメ Windows ソ他ソдみЖъ⑦向んズ適切ス�怪R�藹鬷�サゑクイゆ。
#endif						

#ifndef _WIN32_WINDOWS		// Windows 98 以降ソдみЖъ⑦ズ固有ソ機能ソ使用メ許可ウネエ。
#define _WIN32_WINDOWS 0x0410 // アホメ Windows Me ネギゾガホ以降ソдみЖъ⑦向んズ適切ス�怪R�藹鬷�サゑクイゆ。
#endif

#ifndef _WIN32_IE			// IE 6.0 以降ソдみЖъ⑦ズ固有ソ機能ソ使用メ許可ウネエ。
#define _WIN32_IE 0x0600	// アホメ IE ソ他ソдみЖъ⑦向んズ適切ス�怪R�藹鬷�サゑクイゆ。
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// 一部ソ CString ヵ⑦ЗЬьヱУゾ明示的ザエ。

// 一般的ザ無視ウサパ安全ス MFC ソ警告фЧЙみЖソ一部ソ非表示メ解除ウネエ。
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC ソヵヤれプヂ標準ヵ⑦рみб⑦Ь
#include <afxext.h>         // MFC ソ�^張部分





#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>		// MFC ソ Internet Explorer 4 ヵх⑦ ヵ⑦Ь①みю ДрみЬ
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC ソ Windows ヵх⑦ ヵ⑦Ь①みю ДрみЬ
#endif // _AFX_NO_AFXCMN_SUPPORT









#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

