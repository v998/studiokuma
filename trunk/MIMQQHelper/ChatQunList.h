#ifndef CHATQUNLIST_H
#define CHATQUNLIST_H
class CChatQunList: public CQunListBase {
public:
	static CChatQunList* getInstance();
	virtual void QunOnline(HANDLE hContact);
	virtual void MessageReceived(ipcmessage_t* ipcm);
	virtual void NamesUpdated(ipcmembers_t* ipcms);
	virtual void OnlineMembersUpdated(ipconlinemembers_t* ipcms);
	virtual void MessageSent(HANDLE hContact);
	static INT ChatEventProc(WPARAM wParam, LPARAM lParam);
protected:
	virtual void TabSwitched(CWPRETSTRUCT* cps);
private:
	CChatQunList();
	virtual ~CChatQunList();

	void Refresh();

	static bool fChatInit;
	HANDLE hHookChatEvent;
	LPSTR m_modulename;
};
#endif // CHATQUNLIST_H