// DlgProxy.cpp : ��@��
//

#include "stdafx.h"
#include "FreeImage.h"
#include "ARM.h"
#include "DlgProxy.h"
#include "ARMDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CARMDlgAutoProxy

IMPLEMENT_DYNCREATE(CARMDlgAutoProxy, CCmdTarget)

CARMDlgAutoProxy::CARMDlgAutoProxy()
{
	EnableAutomation();
	
	// �Y�n�O�����ε{�����檺�ɶ��P Automation ����@�Ϊ��ɶ��@�˪��A
	//	�غc�禡�n�I�s AfxOleLockApp�C
	AfxOleLockApp();

	// �g�����ε{���D�������СA���o���ܤ�����s���C�N Proxy ������
	// ���г]�w�����V��ܤ���A�ó]�w���V�� Proxy ����ܤ����^���СC
	ASSERT_VALID(AfxGetApp()->m_pMainWnd);
	if (AfxGetApp()->m_pMainWnd)
	{
		ASSERT_KINDOF(CARMDlg, AfxGetApp()->m_pMainWnd);
		if (AfxGetApp()->m_pMainWnd->IsKindOf(RUNTIME_CLASS(CARMDlg)))
		{
			m_pDialog = reinterpret_cast<CARMDlg*>(AfxGetApp()->m_pMainWnd);
			m_pDialog->m_pAutoProxy = this;
		}
	}
}

CARMDlgAutoProxy::~CARMDlgAutoProxy()
{
	// ��Ҧ����󳣬O�ϥ� Automation �إ߮ɡA�Y�n�������ε{���A
	// 	�ϥθѺc�禡�I�s AfxOleUnlockApp�C���~�A�o�|�R���D��ܤ��
	if (m_pDialog != NULL)
		m_pDialog->m_pAutoProxy = NULL;
	AfxOleUnlockApp();
}

void CARMDlgAutoProxy::OnFinalRelease()
{
	// ������ Automation ����̫᪺�ѦҮɡA�|�I�s OnFinalRelease�C
	// �����O�|�۰ʧR������C�I�s�����O�e�A�Х��[�J�z����һ�
	// �B�~���M�� (cleanup) �{���X�C

	CCmdTarget::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(CARMDlgAutoProxy, CCmdTarget)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CARMDlgAutoProxy, CCmdTarget)
END_DISPATCH_MAP()

// �`�N: �ڭ̥[�J�� IID_IARM ���䴩�H�K�q VBA �䴩���O�w��ô���C
// �� IID �����P .IDL �ɤ��A���[�ܤ��t�{�������� GUID �۲šC

// {B097B5D8-7DB9-482A-B63E-F28F07A53CBE}
static const IID IID_IARM =
{ 0xB097B5D8, 0x7DB9, 0x482A, { 0xB6, 0x3E, 0xF2, 0x8F, 0x7, 0xA5, 0x3C, 0xBE } };

BEGIN_INTERFACE_MAP(CARMDlgAutoProxy, CCmdTarget)
	INTERFACE_PART(CARMDlgAutoProxy, IID_IARM, Dispatch)
END_INTERFACE_MAP()

// �b�����~ {6900BA26-3E90-44B6-8C37-F1F2049FEAF7} �� StdAfx.h ���w�q
// IMPLEMENT_OLECREATE2 ����
IMPLEMENT_OLECREATE2(CARMDlgAutoProxy, "ARM.Application", 0x6900ba26, 0x3e90, 0x44b6, 0x8c, 0x37, 0xf1, 0xf2, 0x4, 0x9f, 0xea, 0xf7)


// CARMDlgAutoProxy �T���B�z�`��
