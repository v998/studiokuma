/*
// stdafx.h : �i�b�����Y�ɤ��]�t�зǪ��t�� Include �ɡA
// �άO�g�`�ϥΫo�ܤ��ܧ󪺱M�ױM�� Include �ɮ�
//

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// �q Windows ���Y�ư����`�ϥΪ�����
#endif

// �p�G�z�������u����������x�A�Эק�U�C�w�q�C
// �Ѧ� MSDN ���o���P���x�����Ȫ��̷s��T�C
#ifndef WINVER				// ���\�ϥ� Windows 95 �P Windows NT 4 (�t) �H�᪩�����S�w�\��C
#define WINVER 0x0400		// �N���ܧ󬰰w�� Windows 98 �M Windows 2000 (�t) �H�᪩���A���ȡC
#endif

#ifndef _WIN32_WINNT		// ���\�ϥ� Windows NT 4 (�t) �H�᪩�����S�w�\��C
#define _WIN32_WINNT 0x0400	// �N���ܧ󬰰w�� Windows 2000 (�t) �H�᪩�����A��ȡC
#endif						

#ifndef _WIN32_WINDOWS		// ���\�ϥ� Windows 98 (�t) �H�᪩�����S�w�\��C
#define _WIN32_WINDOWS 0x0410 // �N���ܧ󬰰w�� Windows Me (�t) �H�᪩���A���ȡC
#endif

#ifndef _WIN32_IE			// ���\�ϥ� IE 4.0 (�t) �H�᪩�����S�w�\��C
#define _WIN32_IE 0x0400	// �N���ܧ󬰰w�� IE 5.0 (�t) �H�᪩���A���ȡC
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// ���T�w�q������ CString �غc�禡

#include <afxwin.h>         // MFC �֤߻P�зǤ���
#include <afxext.h>         // MFC �X�R�\��

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE ���O
#include <afxodlgs.h>       // MFC OLE ��ܤ�����O
#include <afxdisp.h>        // MFC Automation ���O
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>			// MFC ODBC ��Ʈw���O
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>			// MFC DAO ��Ʈw���O
#endif // _AFX_NO_DAO_SUPPORT

#include <afxdtctl.h>		// MFC �䴩�� Internet Explorer 4 �q�α��
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC �䴩�� Windows �q�α��
#endif // _AFX_NO_AFXCMN_SUPPORT

*/
// stdafx.h : �з��Uǳǵ���� �~��ǫ������ ���{�~���U�~��ǫ������ ���{�~��B�e�F�V
// ��Ӧ^����h���B���K���e�q�ħ����s�Q��B����Ǵǣǫ�ēG���U�~��ǫ������ ���{�~��
// �y�O�z���e�@�C

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Windows ��ǿǼ�����p�ϥ����s�M���Q�곡���y���~���e�@�C
#endif

// �U�N���w���s�F�w�q�U�e�R�f�H����ǿ����ǥ�����y���w���Q���s�W�Q�p�Q����X�B�H�U�U�w�q�y�ħ����M���G����C
// ���Q�r����ǿ����ǥ�����R�f�N�@�r���R�k�@�r�̷s�����R�K���M�V�BMSDN �y������M���G����C
#ifndef WINVER				// Windows XP �H���U����Ǵ�����R�T���U�����U�ϥ��y�\�i���e�@�C
#define WINVER 0x0501		// ���s�y Windows �U�L�U����Ǵ����V���R�A���Q���R�ħ����M���G����C
#endif

#ifndef _WIN32_WINNT		// Windows XP �H���U����Ǵ�����R�T���U�����U�ϥ��y�\�i���e�@�C                   
#define _WIN32_WINNT 0x0501	// ���s�y Windows �U�L�U����Ǵ����V���R�A���Q���R�ħ����M���G����C
#endif						

#ifndef _WIN32_WINDOWS		// Windows 98 �H���U����Ǵ�����R�T���U�����U�ϥ��y�\�i���e�@�C
#define _WIN32_WINDOWS 0x0410 // ���s�y Windows Me �e�F�V�D�s�H���U����Ǵ����V���R�A���Q���R�ħ����M���G����C
#endif

#ifndef _WIN32_IE			// IE 6.0 �H���U����Ǵ�����R�T���U�����U�ϥ��y�\�i���e�@�C
#define _WIN32_IE 0x0600	// ���s�y IE �U�L�U����Ǵ����V���R�A���Q���R�ħ����M���G����C
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// �@���U CString ǯ��ǵ����ǫǻ�V���ܪ��N�@�C

// �@�몺�N�L�����M�i�w���Q MFC �Uĵ�i��ǿǷ��Ǵ�U�@���U�D����y�Ѱ����e�@�C
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC �Uǯ�|���o�Z�з�ǯ������������
#include <afxext.h>         // MFC �U�^�i����





#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>		// MFC �U Internet Explorer 4 ǯ���� ǯ���������� Ǳ������
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC �U Windows ǯ���� ǯ���������� Ǳ������
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


