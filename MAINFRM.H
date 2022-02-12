#include "trayicon.h"

// Message ID used for tray notifications
#define WM_MY_TRAY_NOTIFICATION WM_USER+0
// 
#define WM_SET_TRAY_TIP WM_USER+1

//////////////////
// Main frame for TRAYTEST.
//
class CMainFrame : public CFrameWnd {
public:
	CMainFrame();
	virtual ~CMainFrame();
protected:
	DECLARE_DYNAMIC(CMainFrame)
	CStatusBar	m_wndStatusBar;

	CTrayIcon	m_trayIcon;		// my tray icon
	CEdit			m_wndEdit;		// to display tray notifications
	int			m_iWhichIcon;	// 0/1 which HICON to use
	BOOL			m_bShutdown;	// OK to terminate TRAYTEST
	BOOL			m_bShowTrayNotifications;	// display info in main window

	//{{AFX_MSG(CMainFrame)
	afx_msg LRESULT OnTrayNotification(WPARAM wp, LPARAM lp);
  afx_msg LRESULT OnSetTrayTip(WPARAM wp, LPARAM lp);
	afx_msg int	 OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewClear();
	afx_msg void OnViewNotifications();
	afx_msg void OnUpdateViewClear(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewNotifications(CCmdUI* pCmdUI);
	afx_msg void OnClose();
	afx_msg void OnAppSuspend();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
