#pragma once


// CPropertiesGeneral ��ܤ��

class CPropertiesGeneral : public CPropertyPage
{
	DECLARE_DYNAMIC(CPropertiesGeneral)

public:
	CPropertiesGeneral();
	virtual ~CPropertiesGeneral();

// ��ܤ�����
	enum { IDD = IDD_PROPERTIES_SECTION_GENERAL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �䴩

	BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};
