#pragma once

#include "FreeImage.h"
#include "XUnzip.h"

// CPropertiesFiles ��ܤ��

class CPropertiesFiles : public CPropertyPage
{
	DECLARE_DYNAMIC(CPropertiesFiles)

public:
	CPropertiesFiles();
	virtual ~CPropertiesFiles();

// ��ܤ�����
	enum { IDD = IDD_PROPERTIES_FILES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �䴩
	BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedPropPreview();
	static int previewWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static FIBITMAP* fiBitmap;
	static ZIPENTRY* bitmapInfo;
	afx_msg void OnLbnSelchangePropFilelist();
	afx_msg void OnLbnDblclkPropFilelist();
};
