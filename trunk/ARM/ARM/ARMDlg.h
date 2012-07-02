// ARMDlg.h : ���Y�� 
//

#pragma once
#include "afxwin.h"
#include "LoadingDialog.h"
#include "BPCtrlAnchorMap.h"
#include "afxcmn.h"
#include "FileCheckListBox.h"
#include "FreeImage.h"
#include "plugins.h"

#include <map>
#include "afxole.h"
using namespace std;

class CARMDlgAutoProxy;


// CARMDlg ��ܤ��
class CARMDlg : public CDialog
{
	DECLARE_DYNAMIC(CARMDlg);
	friend class CARMDlgAutoProxy;

// �غc
public:
	CARMDlg(CWnd* pParent = NULL);	// �зǫغc�禡
	virtual ~CARMDlg();

// ��ܤ�����
	enum { IDD = IDD_ARM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �䴩


// �{���X��@
protected:
	CARMDlgAutoProxy* m_pAutoProxy;
	HICON m_hIcon;

	BOOL CanExit();

	// ���ͪ��T�������禡
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//afx_msg void OnClose();
	virtual void OnOK();
	virtual void OnCancel();
	DECLARE_MESSAGE_MAP()

	DECLARE_ANCHOR_MAP()
public:
	CFileCheckListBox m_lstFiles;
	afx_msg void OnBnClickedCancel();
	PLUGINLINK* pluginLink;
	HANDLE hEventMM2Execute;

	static CLoadingDialog* m_LoadingDialog;
private:
	//CPicture cp;
	FIBITMAP* fiBitmap;
	BOOL InitPlugins();
	CPluginManager* m_pluginManager;

	static map<UINT,MIRANDASERVICE,less<UINT> > menuservices;

public:
	afx_msg void OnLbnSelchangeFilelist();
	CStatusBar m_status;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	CTabCtrl m_tab;
	afx_msg void OnTcnSelchangeTab(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNmRClickFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	//afx_msg BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnChangeStatus(WPARAM wParam,LPARAM lParam);
	afx_msg void OnFileMenuItems(UINT nID);
	afx_msg void OnListMenuItems(UINT nID);
	afx_msg void OnPopupMenuItems(UINT nID);
	afx_msg void OnHelpMenuItems(UINT nID);
	afx_msg void OnPluginMenuItems(UINT nID);
	afx_msg void OnClbnChkChangeFilelist();
	void LoadSelectionList(LPCTSTR szFileName);
	void SaveSelectionList(LPCTSTR szFileName, BOOL fSaveAll);
	void ProcessSelection(INT nSelectionMode, BOOL fAll);
	void SelectiveToggle(INT nSelectionMode, LPCTSTR szMM2Path, LPCTSTR szBAKPath, arfile* arFile);
	void LoadList();
	static UINT MM2Thread(LPVOID pParam);
	afx_msg void OnStnDblclickPreview();

private:
	static int _svcAddMenuItem(WPARAM,LPARAM);
	static int _svcReadSetting(WPARAM,LPARAM);
	static int _svcReadSettingInt(WPARAM,LPARAM);
	static int _svcWriteSetting(WPARAM,LPARAM);
	static int _svcReadGeneralSetting(WPARAM,LPARAM);
	static int _svcReadGeneralSettingInt(WPARAM,LPARAM);
	static int _svcXUZOpenZip(WPARAM,LPARAM);
	static int _svcXUZFindZipItem(WPARAM,LPARAM);
	static int _svcXUZUnzipItem(WPARAM,LPARAM);
	static int _svcXUZGetZipItem(WPARAM,LPARAM);
	static int _svcXUZCloseZip(WPARAM,LPARAM);
	static int _svcFILoadToHBitmap(WPARAM,LPARAM);
public:
	afx_msg void OnDropFiles(HDROP hDropInfo);
};

/*static UINT BASED_CODE indicators[] =
{
	NULL,
	NULL,
	NULL
};*/
