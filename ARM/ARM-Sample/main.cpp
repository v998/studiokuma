// AR Madness �d�Ҵ���
// �Y�n�}�o�s����ɥi�H�o�Ӵ����ť�
// ���� API �������аѷ� http://www.studiokuma.com/arm/?section=api
#include <windows.h>
#include "plugins.h"
#include "m_bsaslt.h"
#include <stdio.h>
#include <stdlib.h>

PLUGINLINK* pluginLink;

static HINSTANCE hInstance;
static HANDLE hMyEvent;

BOOL WINAPI DllMain(HINSTANCE hInst,DWORD fdwReason,LPVOID lpvReserved)
{
	if (fdwReason==DLL_PROCESS_ATTACH) {
		hInstance=hInst;
	}
	return TRUE;
}

// �U����W�z������W��
#define PLUGIN_NAME "Sample"

// �Ĥ@�� MAKELPARAM �̪��ƭȬO���󪩥��A�Y 1.1.1.0�A�ĤG�ӬO�һ� ARM �����A0.0.0.0 �Y���@�ˬd
PLUGININFO pluginInfo = {PLUGIN_NAME,MAKELPARAM(MAKEWORD(1,1),MAKEWORD(1,0)),MAKELPARAM(MAKEWORD(0,0),MAKEWORD(0,0)),__DATE__" "__TIME__};

// �o�O�Ĥ@�өI�s����ơA����Ʒ|��{��������ƶǵ� ARM�A�Y����Ƥ��s�b�ζǦ^ NULL �ȫh����N���Q���J
extern "C" __declspec(dllexport) PLUGININFO* GetPluginInfo() {
	return &pluginInfo;
}

// ����Ƭ� BSASLT ���� (�������֤�) ���ƥ�B�z��ơA�Ҧ� BSASLT �ƥ�w�q�� m_bsaslt.h
int SpeakerEvent(WPARAM wParam, LPARAM lParam) {
	switch (wParam) {
		// �H�U���@�q�ƥ�
		case BSAS_EVENT_VALUE_POWER_ON: // �������}��
		case BSAS_EVENT_VALUE_POWER_OFF: // ����������
		case BSAS_EVENT_VALUE_TEXT:	// ��ܳ�������r (lParam=��ܦr��)
		case BSAS_EVENT_VALUE_CONFIRMED: // �T�{���u (�Y�}����n���u���6��) (lParam=���u�r��)
		case BSAS_EVENT_VALUE_STOP: // �������

		// �H�U�� LED ��ܫ̨ƥ�
		// case BSAS_EVENT_VALUE_INTENSITY: // �ܧ���ܫG�� (lParam=�G�׾��)

		// �H�U�����n���ƥ�
		// case BSAS_EVENT_VALUE_PLAYAUDIO: // ���񭵮��ɮ� (lParam=�����ɮ׬۹���|�r��)
		// case BSAS_EVENT_VALUE_SETVOLUME: // �ܧ󭵶q (lParam=���q���)

		default:
			return 0; // BSAS_EVENT_RESULT_NOT_FOUND;
	}

	// �Ǧ^�ȡG
	// BSAS_EVENT_RESULT_OK: ���\�B�z�ƥ�
	// BSAS_EVENT_RESULT_NOT_IMPL: ���B�z�ƥ�
	// BSAS_EVENT_RESULT_NOT_FOUND: �S���ƥ�

	return BSAS_EVENT_RESULT_OK;
}

// �o�O Sample\MyService �A�Ȫ��B�z���
int _svcMyService(WPARAM wParam, LPARAM lParam) {
	return 0;
}

// �o�O�ĤG�өI�s����ơA�Цb�o�̶i��A�ȤΨƥ�ŧi�A�Y�Ǧ^�Ȭ� FALSE �ɱN�Q���@��l�ƥ���
// plink �̧t�U���U��ƪ����СA�z�����⥦�]�w�� pluginLink �~�ॿ�`�ϥΤ��ب��
extern "C" __declspec(dllexport) BOOL PluginLoad(PLUGINLINK* plink) {
	OutputDebugString("Initializing " PLUGIN_NAME " Module...\n");
	pluginLink=plink;

	// �w�q�i����L����I�s���A��
	CreateServiceFunction("Sample\\MyService",_svcMyService);
	// �I�s�A�ȮɨϥΥH�U��k (wParam �� lParam ���ϥΪ̩w�q���ǤJ�ѼơA_svcMyService ���Ǧ^�ȱN�@�� CallService ���Ǧ^��)
	// CallService("Sample\\MyService",(WPARAM)0,(LPARAM)0);

	// �w�q�i���h�Ӵ���q�\���ƥ�
	hMyEvent=CreateHookableEvent("Sample\\MyEvent");
	// ���G�ƥ�ɨϥΥH�U��k (wParam �� lParam ���ϥΪ̩w�q���ǤJ�Ѽ�)
	// NotifyEventHook(hMyEvent,(WPARAM)0,(LPARAM)0);

	OutputDebugString(PLUGIN_NAME " Module Loaded\n");
	return TRUE;
}

// �o�O ARM �����e�I�s����ơA�Цb�o������Ҧ��w���t���O����������Ҧ��w�}�Ҫ��ɮ�
extern "C" __declspec(dllexport) BOOL PluginUnload() {
	OutputDebugString("Unloading " PLUGIN_NAME " Module...\n");
	OutputDebugString(PLUGIN_NAME " Module Unloaded\n");
	return TRUE;
}

// �o�O�ĤT�өI�s����ơA�Цb�o�̶i��ĤG��������l�� (�Ҧp�q�\��L���󪺨ƥ�)
// �`�N�I�z����b PluginLoad �̭q�\�ƥ�A�]���I�s PluginLoad �ɨå���l�ƩҦ�����
extern "C" __declspec(dllexport) BOOL ModulesLoaded() {
	// BSASLT �@�� 3 �Өƥ�i�ѭq�\
	// BSAS_EVENT_OUT_CONSOLE: �N�����}�����λP�D���x�������ƥ�
	// BSAS_EVENT_OUT_DISPLAY: �N�����}�����λP LED ��ܫ̦������ƥ�
	// BSAS_EVENT_OUT_SPEAKER: �N�����}�����λP���n���������ƥ�
	HookEvent(BSAS_EVENT_OUT_SPEAKER,SpeakerEvent);

	return TRUE;
}

