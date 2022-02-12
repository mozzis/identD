 ////////////////////////////////////////////////////////////////
// TRAYTEST Copyright 1996 Microsoft Systems Journal. 
// If this program works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
// TRAYTEST illustrates how to use CTrayIcon. 
// All the activity takes place in MainFrm.cpp.

#include "stdafx.h"
#include <winsock.h>
#include <atlbase.h>
#include "identd.h"
#include "mainfrm.h"

CMyApp theApp;

BEGIN_MESSAGE_MAP(CMyApp, CWinApp)
	//{{AFX_MSG_MAP(CMyApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

const int PORT = 113;
const int ALARM_TIME = 5;
const int TIME_OUT = 180;    /* login timeout in sec - can't be < ALARM_TIME */

UINT __cdecl WinsockThreadProc(LPVOID lParam);

// strtolower() converts str to lowercase
static void strtolower(TCHAR *str) 
{
  if (str==NULL) return;
  while (*str) {
    *str=_tolower(*str);
    str++;
    }
}

// allocates some space for the user and puts it on the list
CONNECTION *CMyApp::CreateUser(void) 
{
  if (!m_pHead) 
    {
    m_pHead = new CONNECTION ;
    if (m_pHead) 
      {
      memset(m_pHead, 0, sizeof(CONNECTION));
      m_pTail = m_pHead;
      m_pHead->next = NULL;
      }
    return m_pHead;
    }
  
  CONNECTION *pNewUser = new CONNECTION;
  if (pNewUser)
    {
    memset(pNewUser, 0, sizeof(CONNECTION));
    m_pTail->next = pNewUser;
    pNewUser->next = NULL;
    m_pTail = pNewUser;
    }
  return pNewUser;
}

// UserQuit(CONNECTION *user)
//  * closes the connection and removes it from the list
int CMyApp::UserQuit(CONNECTION *pUser) 
{
  CONNECTION *i, *j;
  
  if (pUser==NULL) return 1;
  if (pUser->sock > -1) 
    {
    closesocket(pUser->sock);
    }
  if (pUser == m_pHead) 
    {
    if (m_pTail == m_pHead) 
      {
      m_pHead = m_pTail = NULL;
      delete pUser;
      return 0; /* return 0 in only case -- empty list */
      }
    m_pHead = m_pHead->next;
    delete pUser;
    return 1;
    }
  for (i = m_pHead; i->next != pUser && i->next != NULL; i = i->next);
  j = i->next;
  if (j != pUser) 
    {
    TRACE0("ERROR: Programmer error: Connection list is corrupted.\r\n");
    PostQuitMessage(1);
    }
  if( j == m_pTail ) 
    m_pTail = i;
  i->next = j->next;
  j->next = NULL;
  free(pUser);
  return 1;
}

void CMyApp::CheckTimeout() 
{
  time_t tm = time((time_t *)0);
  CONNECTION *user = m_pHead;
  while( user != NULL ) 
    {
    for (user = m_pHead; user != NULL; user = user->next) 
      {
      int secs = (int)tm - user->last_input;
      if (secs >= TIME_OUT) 
        {
        TRACE("ERROR: %s timed out\r\n", user->site);
        UserQuit(user);
        break; // restart user scanning
        } // if
      } // for each user
    } // while scanning is complete
}


// send string down socket
void inline write_connection(CONNECTION *pUser, LPCTSTR pStr) 
{
  if (!pUser) 
    return;
  if (pUser->sock > -1) 
    send(pUser->sock, pStr, _tcslen(pStr), 0);
}
  
CMyApp::CMyApp()
{
  m_pHead = NULL,
  m_pTail = NULL;
  m_bResolvForLog = false;
  m_connect_count = 0;
}

// Check to see if this is the first/only instance of the program
/////////////////////////////////////////////////////////////////////////////
BOOL IsFirstInstance(HINSTANCE hInstance)
{
  // define a global semaphore to see if an instance is already running
  HANDLE gbl_hSem = 
    CreateSemaphore(NULL, 0, 1, "IdentDAlreadyRunning");
  // if the semaphore handle returned is valid, but the last error
  // indicates that it already exists, then activate the previous instance
  // and kill this instance
  if (gbl_hSem != NULL && GetLastError() == ERROR_ALREADY_EXISTS)
    {
    CloseHandle(gbl_hSem);
    return 0; // flag to kill this instance
    }
  else
    return 1; // flag to keep runnning this instance
}

BOOL CMyApp::InitInstance()
{
  if (!IsFirstInstance(AfxGetInstanceHandle()))
    return FALSE;

	// Create main frame window (don't use doc/view stuff)
	// 
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	pMainFrame->ShowWindow(SW_HIDE);
	pMainFrame->UpdateWindow();
	m_pMainWnd = pMainFrame;

	// Install path in registry so it runs the next time around
  TCHAR szBuff[_MAX_PATH];
	VERIFY(::GetModuleFileName(m_hInstance, szBuff, _MAX_PATH));
  CRegKey cKey;
  DWORD dwRes = cKey.Create(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"));
  if (0 == dwRes)
    {
    cKey.SetValue(szBuff, _T("Network Identifier"));
    cKey.Close();
    }

  TCHAR  uname[sizeof(m_userid)];
  DWORD  ulen = sizeof(m_userid);

  /* get username from somewhere */
  if (GetUserName(uname, &ulen) ) 
    strncpy(m_userid, uname, ulen);
  else strcpy(m_userid, "unknown");
  
  WORD wVersionRequested = MAKEWORD(1, 1);
  WSADATA wsaData;
  
  int err = WSAStartup( wVersionRequested, &wsaData );
  if ( err != 0 ) {
    /* Tell the user that we couldn't find a useable winsock.dll */
    TRACE("FAIL: Couldn't find suitable winsock.dll\n");
    PostQuitMessage(1);;
    }
  
  /* Confirm that the Windows Sockets DLL supports 1.1.*/
  /* Note that if the DLL supports versions greater    */
  /* than 1.1 in addition to 1.1, it will still return */
  /* 1.1 in wVersion since that is the version we      */
  /* requested.                                        */
  
  if (HIBYTE(wsaData.wVersion) != 1 ||
    LOBYTE(wsaData.wVersion ) != 1 ) {
    /* Tell the user that we couldn't find a useable winsock.dll */
    TRACE("FAIL: Version 1.1 of winsock.dll is required\n");
    WSACleanup( );
    PostQuitMessage(1);;
    }
  
  /* The Windows Sockets DLL is acceptable.  Proceed.  */

  TCHAR localhost[80];     /* local hostname */
  /* get local hostname */
  gethostname(localhost, 80);
  strtolower(localhost);

  if ((m_listen_sock=socket(AF_INET, SOCK_STREAM, 0))==-1) 
    {
    TRACE("FAIL: Couldn't open listen socket\r\n");
    PostQuitMessage(1);;
    }
  
  /* Allow reboots even with TIME_WAITS etc on port */
  int on = 1;
  setsockopt(m_listen_sock, SOL_SOCKET, SO_REUSEADDR, (TCHAR *)&on, sizeof(on));

  struct sockaddr_in bind_addr;

  bind_addr.sin_family = AF_INET;
  bind_addr.sin_addr.s_addr = INADDR_ANY;
  bind_addr.sin_port = htons(PORT);
  if (bind(m_listen_sock, (struct sockaddr *)&bind_addr, sizeof(struct sockaddr_in))==-1) {
    TRACE("FAIL: Couldn't bind to port\r\n");
    PostQuitMessage(1);;
    }
  if (listen(m_listen_sock, 20) == -1) 
    {
    TRACE("FAIL: Listen error\r\n");
    PostQuitMessage(1);;
    }
  
  CString csMsg;
  csMsg.Format("%s : %d", m_userid, m_connect_count);
  pMainFrame->SendMessage(WM_SET_TRAY_TIP, 0, (LPARAM)(LPCTSTR)csMsg);
  // Start the thread to handle connections to the listening socket
  m_bSockThreadContinue = true;
  AfxBeginThread(WinsockThreadProc, this);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Use this override to clean up when program terminates
int CMyApp::ExitInstance(void)
{
  m_bSockThreadContinue = false; // signal thread time to exit
  if(WSAIsBlocking()) 
    WSACancelBlockingCall();
  WSACleanup();
  return CWinApp::ExitInstance();
}


UINT __cdecl WinsockThreadProc(LPVOID lParam)
{
  CMyApp *pApp = (CMyApp *)lParam;
  static fd_set readmask;
  static CString csMsg;
  
  while (pApp->m_bSockThreadContinue) // 'infinite' loop to listen and respond
    {
    FD_ZERO(&readmask);
    CONNECTION *pUser;
    for (pUser = pApp->m_pHead; pUser != 0; pUser = pUser->next)
      {
      if (pUser->sock != -1) 
        FD_SET(pUser->sock, &readmask);
      }
    FD_SET(pApp->m_listen_sock, &readmask);
      
    // wait for connection (blocking call)
    // (assumes errors are nice and not nasty) */
    int i = select(FD_SETSIZE, &readmask, 0, 0, NULL);
    // exit thread if program ended during block
    if (!pApp->m_bSockThreadContinue)
      break; // cleanup and exit thread
    if (SOCKET_ERROR == i) 
      {
      i = WSAGetLastError();
      break; // cleanup and exit thread
      }
    if (!i) 
      { 
      pApp->CheckTimeout(); 
      break; // cleanup and exit thread
      }
    
    /* check for connection to listen socket */
    if (FD_ISSET(pApp->m_listen_sock, &readmask)) 
      {
      struct sockaddr_in acc_addr;
      int size = sizeof(sockaddr);

      pApp->m_accept_sock = 
        accept(pApp->m_listen_sock, (struct sockaddr *)&acc_addr, &size);
      if (INVALID_SOCKET == pApp->m_accept_sock)
        {
        TRACE("ERROR: Invalid socket error.\r\n");
        break; //cleanup and exit thread
        }
      if ((pUser = pApp->CreateUser()) == NULL) 
        {
        TRACE("ERROR: Reached connection limit.\r\n");
        closesocket(pApp->m_accept_sock);
        break; // cleanup and exit thread
        }
    
      /* get remote internet site */
      TCHAR site_num[80];
      strcpy(site_num, inet_ntoa(acc_addr.sin_addr));
      UINT addr = inet_addr(site_num);
      strcpy(pUser->site, site_num);
      if (pApp->m_bResolvForLog)
        {
        struct hostent *host = gethostbyaddr((TCHAR *)&addr, 4, AF_INET);
        if (host)
          strncpy(pUser->site, host->h_name, 79);
        }
      
      strtolower(pUser->site);
      pUser->port = ntohs(acc_addr.sin_port);
    
      pUser->sock = pApp->m_accept_sock;
      pUser->last_input = time((time_t *)0);
      }
    
    /* need extra cycle to be able to restart scanning if user list altered */
    pUser = pApp->m_pHead;
    while(pUser != NULL) 
      {
      /** cycle through connections **/
      for (pUser = pApp->m_pHead; pUser; pUser = pUser->next) 
        {
        /* if no data waiting on socket, go to the next socket */
        if (!FD_ISSET(pUser->sock, &readmask)) 
          continue; // next user
        /* see if client has closed socket */
        TCHAR inpstr[ARR_SIZE] = { '\0' };
        int len = recv(pUser->sock, inpstr, sizeof(inpstr), 0);

        if (len <= 0) 
          {
          pApp->UserQuit(pUser);
          break; /* need to quit cycle anyway to restart list from beginnig */
          }
      
        inpstr[len]='\0';
        pUser->last_input=time((time_t *)0);  /* ie now */
        int k = pUser->bufpos;
        for (i=0; i < len; ++i) 
          {
          BYTE c = (BYTE)inpstr[i];
          /* handle eol */
          if ('\n' == c || '\r' == c || !c || k+5 >= ARR_SIZE) 
            goto GOT_LINE;
        
          /* handle non 7-bit ascii chars (not RFC compliant) */
          /* if ( c<32 || c>126 ) continue; */
        
          /* copy the char into buffer */
          pUser->buffer[k++]=c;
          }
        pUser->bufpos = k;
        continue; // next user
GOT_LINE:
        if (k+5>ARR_SIZE) 
          {
          csMsg.Format("1, 1: ERROR : UNKNOWN-ERROR\r\n");
          write_connection(pUser, csMsg);
          TRACE("ERROR: %s overflowed the buffer\r\n", pUser->site);
          pApp->UserQuit(pUser);
          break; /* need to quit cycle anyway to restart list from beginnig */
          }
      
        pUser->buffer[k]='\0';
        int port1 = 0, port2 = 0;
        for ( LPTSTR pC = pUser->buffer; *pC && *pC !=','; pC++);
        if (*pC) 
          {
          *pC = '\0';
          port1 = atoi(pUser->buffer);
          port2 = atoi(pC + 1);
          }
      
        if (!port1 || !port2 || (unsigned)port1>65535 || (unsigned)port2>65535) {
          csMsg.Format("%d, %d : ERROR : INVALID-PORT\r\n", port1, port2);
          write_connection(pUser, csMsg);
          TRACE("ERROR: %s invalid port %d, %d\r\n", pUser->site, port1, port2);
          pApp->UserQuit(pUser);
          break; /* need to quit cycle anyway to restart list from beginnig */
          }
      
        /* here we could do stuff */
        csMsg.Format("%d, %d : USERID : %s : %s\r\n", port1, port2, "UNIX", pApp->m_userid);
        write_connection(pUser, csMsg);
        TRACE("GOOD: success for %s on ports %d, %d\r\n", pUser->site, port1, port2);
        pApp->UserQuit(pUser);
      
        /* update tip */
        pApp->m_connect_count++;
        csMsg.Format("%s : %d", pApp->m_userid, pApp->m_connect_count);
        AfxGetMainWnd()->SendMessage(WM_SET_TRAY_TIP, 0, (LPARAM)(LPCTSTR)csMsg);
      
        break; /* need to quit cycle to restart list from beginnig */
        } /* end of foreach user */
      } /* end of while scanning not complete*/
    } // while thread continue flag set
  return 0;
}

void CMyApp::OnAppAbout()
{
	CDialog(IDD_ABOUTBOX).DoModal();
}

