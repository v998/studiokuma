#include "../arm/plugins.h"

// �U�C ifdef �϶��O�إߥ����H��U�q DLL �ץX���зǤ覡�C
// �o�� DLL �����Ҧ��ɮ׳��O�ϥΩR�O�C���ҩw�q ARMBGDISPLAY_EXPORTS �Ÿ��sĶ���C
//  ����ϥγo�� DLL ���M�׳������w�q�o�ӲŸ��C�o�˪��ܡA��l�{���ɤ��]�t�o�ɮת������L�M��
// �|�N ARMBGDISPLAY_API �禡�����q DLL �פJ���A�ӳo�� DLL �h�|�N�o�ӥ����w�q���Ÿ������ץX���C
#ifdef ARMBGDISPLAY_EXPORTS
#define ARMBGDISPLAY_API __declspec(dllexport)
#else
#define ARMBGDISPLAY_API __declspec(dllimport)
#endif

extern "C" {
	ARMBGDISPLAY_API PLUGININFO* GetPluginInfo();
	ARMBGDISPLAY_API BOOL PluginLoad(PLUGINLINK*);
	ARMBGDISPLAY_API BOOL PluginUnload() ;
	ARMBGDISPLAY_API BOOL ModulesLoaded();
}
