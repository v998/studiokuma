#pragma once


// CSettingsOptions ��ܤ��

class CSettingsOptions : public CPropertyPage
{
	DECLARE_DYNAMIC(CSettingsOptions)

public:
	CSettingsOptions();
	virtual ~CSettingsOptions();

// ��ܤ�����
	enum { IDD = IDD_SETUP_OPTIONS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �䴩
	BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	void SaveSettings();
	BOOL m_recycle;
	BOOL m_minimize;
	BOOL m_noyikes;
	int m_args;
	BOOL m_notify;
};
