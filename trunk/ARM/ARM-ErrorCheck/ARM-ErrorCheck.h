// �U�C ifdef �϶��O�إߥ����H��U�q DLL �ץX���зǤ覡�C
// �o�� DLL �����Ҧ��ɮ׳��O�ϥΩR�O�C���ҩw�q ARMERRORCHECK_EXPORTS �Ÿ��sĶ���C
// ����ϥγo�� DLL ���M�׳������w�q�o�ӲŸ��C
// �o�ˤ@�ӡA��l�{���ɤ��]�t�o�ɮת������L�M��
// �|�N ARMERRORCHECK_API �禡�����q DLL �פJ���A�ӳo�� DLL �h�|�N�o�ǲŸ�����
// �ץX���C
#ifdef ARMERRORCHECK_EXPORTS
#define ARMERRORCHECK_API __declspec(dllexport)
#else
#define ARMERRORCHECK_API __declspec(dllimport)
#endif
