////////////////////////////////////////////////////////////////
// TRAYTEST Copyright 1996 Microsoft Systems Journal. 
// See TRAYTEST.CPP for description of program.
// 
#include "resource.h"

const int ARR_SIZE = 1024;
typedef struct timeval timeval;

typedef struct CONNECTION
{
  CONNECTION *next;             /* next connection in the list */
  TCHAR buffer[ARR_SIZE], /* input buffer */
        site[80];         /* remote site for this connection */
  int   sock,             /* socket handle */
        bufpos,
        last_input;       /* last input time, so we can disconnect idle connections */
  unsigned int port;      /* remote tcp port for this connection */

} CONNECTION;

class CMyApp : public CWinApp {
public:
  CMyApp();
  virtual BOOL InitInstance();
  virtual int ExitInstance(void);

  TCHAR m_userid[512];

  CONNECTION *m_pHead,   /* the beginning of the list of connections */
             *m_pTail;   /* the end of the list of connections */
  int        m_listen_sock,
             m_accept_sock,
             m_connect_count;
  bool m_bResolvForLog;
  bool m_bSockThreadContinue;

  CONNECTION *CreateUser(void);
  int UserQuit(CONNECTION *user);
  void CheckTimeout();

public:
  //{{AFX_MSG(CMyApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
