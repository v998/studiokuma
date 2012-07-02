#pragma once


// CSettingsGeneral ��ܤ��

class CSettingsGeneral : public CPropertyPage
{
	DECLARE_DYNAMIC(CSettingsGeneral)

public:
	CSettingsGeneral();
	virtual ~CSettingsGeneral();

// ��ܤ�����
	enum { IDD = IDD_SETUP_SECTION_GENERAL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �䴩
	BOOL OnInitDialog();
	afx_msg int OnBffmInitialized(LPARAM lParam,LPARAM lpData);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedSetBrowse1();
	afx_msg void OnBnClickedSetBrowse2();
	void SaveSettings();
	CString m_mm2path;
	CString m_bakpath;
	afx_msg void OnBnClickedSetDefault();

private:
	void SearchMM2(LPSTR pszPath);
};
