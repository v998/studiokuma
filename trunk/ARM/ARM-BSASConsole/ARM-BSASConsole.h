// ARM-BSASConsole.h : ARM-BSASConsole DLL ���D�n���Y��
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// �D�n�Ÿ�
#include "ConsoleDialog.h"

// CARMBSASConsoleApp
// �o�����O����@�аѾ\ ARM-BSASConsole.cpp
//

class CARMBSASConsoleApp : public CWinApp
{
public:
	CARMBSASConsoleApp();

	virtual ~CARMBSASConsoleApp();

// �мg
public:
	virtual BOOL InitInstance();
	void SetupConsole(char* fileName);

	DECLARE_MESSAGE_MAP()

private:
	CConsoleDialog* m_consoleDialog;
	RECT m_rect;
};

#include "../arm/plugins.h"

// �U�C ifdef �϶��O�إߥ����H��U�q DLL �ץX���зǤ覡�C
// �o�� DLL �����Ҧ��ɮ׳��O�ϥΩR�O�C���ҩw�q ARMBGDISPLAY_EXPORTS �Ÿ��sĶ���C
//  ����ϥγo�� DLL ���M�׳������w�q�o�ӲŸ��C�o�˪��ܡA��l�{���ɤ��]�t�o�ɮת������L�M��
// �|�N API �禡�����q DLL �פJ���A�ӳo�� DLL �h�|�N�o�ӥ����w�q���Ÿ������ץX���C
#ifdef ARMBSASCONSOLE_EXPORTS
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#endif

extern "C" {
	API PLUGININFO* GetPluginInfo();
	API BOOL PluginLoad(PLUGINLINK*);
	API BOOL PluginUnload() ;
	API BOOL ModulesLoaded();
}
