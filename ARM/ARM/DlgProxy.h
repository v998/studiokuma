// DlgProxy.h: ���Y��
//

#pragma once

class CARMDlg;


// CARMDlgAutoProxy �R�O�ؼ�

class CARMDlgAutoProxy : public CCmdTarget
{
	DECLARE_DYNCREATE(CARMDlgAutoProxy)

	CARMDlgAutoProxy();           // �ʺA�إߩҨϥΪ��O�@�غc�禡

// �ݩ�
public:
	CARMDlg* m_pDialog;

// �@�~
public:

// �мg
	public:
	virtual void OnFinalRelease();

// �{���X��@
protected:
	virtual ~CARMDlgAutoProxy();

	// ���ͪ��T�������禡

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(CARMDlgAutoProxy)

	// ���ͪ� OLE ���������禡

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

