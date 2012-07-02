#pragma once

#include "resource.h"		// �D�n�Ÿ�

// �H�U�U ifdef ����ǿǫ�V DLL ���p�UǤǫǵ�������y�e���R�@�r��ǫ���y�@���@�r�F�h�U 
// �@�몺�Q��k�N�@�C���U DLL �X�U�@�`�M�U���{�~���V�Bǯ������ ���~���N�w�q���s�F ARMDISPLAY_EXPORTS
// ǳ�������Nǯ�����~�����s�e�@�C���Uǳ�������V�B���U DLL �y��������Ǵǣǫ���N�w�q�@�r���O�V�N���e�B�z�C
// ǹ��ǵ���{�~�������U���{�~���y�t�z�N���r�L�U����Ǵǣǫ���V�B 
// ARMDISPLAY_API �k���y DLL ���p�~�����������s�F�O���Q�@�U�R�f���B���U DLL �V�B���U��ǫ���N�w�q���s�F
// ǳ�������yǤǫǵ���������s�F�O���Q���e�@�C
#ifdef ARMDISPLAY_EXPORTS
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#endif

#include "../arm/plugins.h"
#include "DisplayDialog.h"

class CARMDisplayApp : public CWinApp
{
public:
	CARMDisplayApp();

	virtual ~CARMDisplayApp();

	// �мg
public:
	virtual BOOL InitInstance();
	void SetupDisplay(char* fileName);

	DECLARE_MESSAGE_MAP()

private:
	CDisplayDialog* m_displayDialog;
};


extern "C" {
	API PLUGININFO* GetPluginInfo();
	API BOOL PluginLoad(PLUGINLINK*);
	API BOOL PluginUnload() ;
	API BOOL ModulesLoaded();
}

// ���Uǫ��ǵ�V ARM-Display.dll ���pǤǫǵ���������s�e���F�C
/*class ARMDISPLAY_API CARMDisplay {
public:
	CARMDisplay(void);
	// TODO: ��ǹǿ���y�����R�l�[���M���G����C
};

extern ARMDISPLAY_API int nARMDisplay;

ARMDISPLAY_API int fnARMDisplay(void);*/

//void SetupConsole(char* fileName);
