// ListboxOleDropTarget.cpp : ��@��
//

#include "stdafx.h"
#include "ARM.h"
#include "ListboxOleDropTarget.h"
#include ".\listboxoledroptarget.h"


// CListboxOleDropTarget

IMPLEMENT_DYNAMIC(CListboxOleDropTarget, COleDropTarget)
CListboxOleDropTarget::CListboxOleDropTarget()
: m_receiverWnd(NULL)
{
}

CListboxOleDropTarget::~CListboxOleDropTarget()
{
}


BEGIN_MESSAGE_MAP(CListboxOleDropTarget, COleDropTarget)
END_MESSAGE_MAP()



// CListboxOleDropTarget �T���B�z�`��

DROPEFFECT CListboxOleDropTarget::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	// TODO: �b���[�J�S�w���{���X�M (��) �I�s�����O
	if (pDataObject->IsDataAvailable(CF_TEXT,NULL)) {
		//m_receiverWnd->SendMessage(WM_DROPFILES
	}
	return COleDropTarget::OnDragOver(pWnd, pDataObject, dwKeyState, point);
}

void CListboxOleDropTarget::setReceiverWnd(CWnd* receiverWnd)
{
	m_receiverWnd=receiverWnd;
}
