// stdafx.h : �з��Uǳǵ���� �~��ǫ������ ���{�~���U�~��ǫ������ ���{�~��B�e�F�V
// ��Ӧ^����h���B���K���e�q�ħ����s�Q��B����Ǵǣǫ�ēG���U�~��ǫ������ ���{�~��
// �y�O�z���e�@�C
//

#pragma once

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
#define _WIN32_IE 0x0600	// ���s�y IE. �U�L�U����Ǵ����V���R�A���Q���R�ħ����M���G����C
#endif

#define WIN32_LEAN_AND_MEAN		// Windows ��ǿǼ�����p�ϥ����s�M���Q�곡���y���~���e�@�C
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// �@���U CString ǯ��ǵ����ǫǻ�V���ܪ��N�@�C

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Windows ��ǿǼ�����p�ϥ����s�M���Q�곡���y���~���e�@�C
#endif

#include <afx.h>
#include <afxwin.h>         // MFC �Uǯ�|���o�Z�з�ǯ������������
#include <afxext.h>         // MFC �U�^�i����
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>		// MFC �U Internet Explorer 4 ǯ���� ǯ���������� Ǳ������
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC �U Windows ǯ���� ǯ���������� Ǳ������
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <iostream>
// Windows ��ǿǼ�� ���{�~��:
#include <windows.h>



// TODO: ����Ǭ�����R���n�Q�l�[��ǿǼ���y�����N������M���G����C
