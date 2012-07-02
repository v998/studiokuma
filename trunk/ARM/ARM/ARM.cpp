// ARM.cpp : �w�q���ε{�������O�欰�C
//

#include "stdafx.h"
#include "FreeImage.h"
#include "ARM.h"
#include "ARMDlg.h"
#include ".\arm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CARMApp

BEGIN_MESSAGE_MAP(CARMApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CARMApp �غc

CARMApp::CARMApp(): m_exiting(false)
{
	// TODO: �b���[�J�غc�{���X�A
	// �N�Ҧ����n����l�]�w�[�J InitInstance ��
}


// �Ȧ����@�� CARMApp ����

CARMApp theApp;

const GUID CDECL BASED_CODE _tlid =
		{ 0x1186C0A3, 0x12B, 0x4BBF, { 0x93, 0x36, 0x19, 0xA8, 0xF8, 0x98, 0x3E, 0x64 } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;


// CARMApp ��l�]�w

BOOL CARMApp::InitInstance()
{
	// ���p���ε{����T�M����w�ϥ� ComCtl32.dll 6.0 (�t) �H�᪩��
	// �H�ҥε�ı�Ƽ˦��A�h Windows XP �ݭn InitCommonControls()�C�_�h���ܡA
	// ����������إ߱N���ѡC
	InitCommonControls();
	SetProcessAffinityMask(GetCurrentProcess(),1);

	CWinApp::InitInstance();

	// ��l�� OLE �{���w
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	// �зǪ�l�]�w
	// �p�G�z���ϥγo�ǥ\��åB�Q��ֳ̫᧹�����i�����ɤj�p�A�z�i�H�q�U�C
	// �{���X�������ݭn����l�Ʊ`���A�ܧ��x�s�]�w�Ȫ��n�����X
	// TODO: �z���ӾA�׭ק惡�r�� (�Ҧp�A���q�W�٩β�´�W��)
	SetRegistryKey(_T("���� AppWizard �Ҳ��ͪ����ε{��"));
	// ��R Automation �� reg/unreg �Ѽƪ��R�O�C�C
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// �w�ϥ� /Embedding �� /Automation �ѼƱҰ����ε{���C
	// �N���ε{����@ Automation ���A������C
	if (cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
	{
		// �g�� CoRegisterClassObject() �n�����O�]�ơC
		COleTemplateServer::RegisterAll();
	}
	// �w�ϥ� /Unregserver �� /Unregister �ѼƱҰ����ε{���C�q�n���������ءC
	else if (cmdInfo.m_nShellCommand == CCommandLineInfo::AppUnregister)
	{
		COleObjectFactory::UpdateRegistryAll(FALSE);
		AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor);
		return FALSE;
	}
	// �w��W�ηf�t��L�Ѽ� (�p /Register �� /Regserver) �Ұ����ε{���C
	// ��s�n�����ءA�]�A���O�{���w�C
	else
	{
		COleObjectFactory::UpdateRegistryAll();
		AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid);
		if (cmdInfo.m_nShellCommand == CCommandLineInfo::AppRegister)
			return FALSE;
	}

	CARMDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: �b����m��ϥ� [�T�w] �Ӱ���ϥι�ܤ����
		// �B�z���{���X
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �b����m��ϥ� [����] �Ӱ���ϥι�ܤ����
		// �B�z���{���X
	}

	// �]���w�g������ܤ���A�Ǧ^ FALSE�A�ҥH�ڭ̷|�������ε{���A
	// �ӫD���ܶ}�l���ε{�����T���C
	return FALSE;
}

