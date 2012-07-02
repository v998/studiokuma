#pragma once

extern struct _arfile;

// CLoadingDialog ��Eܤ趁E

class CLoadingDialog : public CDialog
{
	DECLARE_DYNAMIC(CLoadingDialog)

public:
	CLoadingDialog(CWnd* pParent = NULL);   // �зǫغc�禡
	virtual ~CLoadingDialog();

// ��Eܤ����E�
	enum { IDD = IDD_LOADING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �䴩
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	//afx_msg void OnBnClickedCancel();
	void OnOK();
	void OnCancel();

private:
	static UINT WorkerThread(LPVOID pParam);
	static void MakeInfo(arfile* node);
	static arfile* currentAR;
	static bool fEnumComplete;
};

void FreeARFile(arfile* node);
