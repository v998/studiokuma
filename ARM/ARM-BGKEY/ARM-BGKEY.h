#ifndef ARM_BGKEY_H
#define ARM_BGKEY_H
#include "../arm/plugins.h"

// �U�C ifdef �϶��O�إߥ����H��U�q DLL �ץX���зǤ覡�C
// �o�� DLL �����Ҧ��ɮ׳��O�ϥΩR�O�C���ҩw�q ARMBGKEY_EXPORTS �Ÿ��sĶ���C
//  ����ϥγo�� DLL ���M�׳������w�q�o�ӲŸ��C�o�˪��ܡA��l�{���ɤ��]�t�o�ɮת������L�M��
// �|�N ARMBGKEY_API �禡�����q DLL �פJ���A�ӳo�� DLL �h�|�N�o�ӥ����w�q���Ÿ������ץX���C
#ifdef ARMBGKEY_EXPORTS
#define ARMBGKEY_API __declspec(dllexport)
#else
#define ARMBGKEY_API __declspec(dllimport)
#endif

extern "C" {
	ARMBGKEY_API PLUGININFO* GetPluginInfo();
	ARMBGKEY_API BOOL PluginLoad(PLUGINLINK*);
	ARMBGKEY_API BOOL PluginUnload() ;
	ARMBGKEY_API BOOL ModulesLoaded();
}
#endif //ARM_BGKEY_H