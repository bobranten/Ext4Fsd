#ifndef _WTSAPI32_H
#define _WTSAPI32_H
#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if (_WIN32_WINNT >= 0x0501)
  // session notification message
#define WM_WTSSESSION_CHANGE		0x02B1

typedef enum _WTS_INFO_CLASS {
  WTSInitialProgram       = 0,
  WTSApplicationName      = 1,
  WTSWorkingDirectory     = 2,
  WTSOEMId                = 3,
  WTSSessionId            = 4,
  WTSUserName             = 5,
  WTSWinStationName       = 6,
  WTSDomainName           = 7,
  WTSConnectState         = 8,
  WTSClientBuildNumber    = 9,
  WTSClientName           = 10,
  WTSClientDirectory      = 11,
  WTSClientProductId      = 12,
  WTSClientHardwareId     = 13,
  WTSClientAddress        = 14,
  WTSClientDisplay        = 15,
  WTSClientProtocolType   = 16,
  WTSIdleTime             = 17,
  WTSLogonTime            = 18,
  WTSIncomingBytes        = 19,
  WTSOutgoingBytes        = 20,
  WTSIncomingFrames       = 21,
  WTSOutgoingFrames       = 22,
  WTSClientInfo           = 23,
  WTSSessionInfo          = 24
} WTS_INFO_CLASS;

typedef enum _WTS_CONNECTSTATE_CLASS {
  WTSActive,
  WTSConnected,
  WTSConnectQuery,
  WTSShadow,
  WTSDisconnected,
  WTSIdle,
  WTSListen,
  WTSReset,
  WTSDown,
  WTSInit 
} WTS_CONNECTSTATE_CLASS;

  // session notification message flags
#define WTS_CONSOLE_CONNECT			0x1
#define WTS_CONSOLE_DISCONNECT		0x2
#define WTS_REMOTE_CONNECT			0x3
#define WTS_REMOTE_DISCONNECT		0x4
#define WTS_SESSION_LOGON			0x5
#define WTS_SESSION_LOGOFF			0x6
#define WTS_SESSION_LOCK			0x7
#define WTS_SESSION_UNLOCK			0x8
#define WTS_SESSION_REMOTE_CONTROL	0x9

  // WTSRegisterSessionNotifiction flags
#define NOTIFY_FOR_THIS_SESSION		0
#define NOTIFY_FOR_ALL_SESSIONS		1

BOOL WINAPI WTSRegisterSessionNotification(HWND hWnd, DWORD dwFlags);
BOOL WINAPI WTSUnRegisterSessionNotification(HWND hWnd);
BOOL WINAPI WTSQueryUserToken(ULONG SessionId, PHANDLE pToken);

#endif /* _WIN32_WINNT >= 0x0501 */

#if (_WIN32_WINNT >= 0x0500)

typedef struct _WTS_SESSION_INFOW {
  DWORD SessionId;
  LPWSTR pWinStationName;
  WTS_CONNECTSTATE_CLASS State;
} WTS_SESSION_INFOW, *PWTS_SESSION_INFOW;

typedef struct _WTS_SESSION_INFOA {
  DWORD SessionId;
  LPSTR pWinStationName;
  WTS_CONNECTSTATE_CLASS State;
} WTS_SESSION_INFOA, *PWTS_SESSION_INFOA;

#ifdef UNICODE
#define WTS_SESSION_INFO	WTS_SESSION_INFOW
#define PWTS_SESSION_INFO	PWTS_SESSION_INFOW
#else
#define WTS_SESSION_INFO	WTS_SESSION_INFOA
#define PWTS_SESSION_INFO	PWTS_SESSION_INFOA
#endif

  // WTSWaitSystemEvent local server handle
#define WTS_CURRENT_SERVER_HANDLE 0
#define WTS_CURRENT_SESSION       ((DWORD)-1)

  // WTSWaitSystemEvent flags
#define WTS_EVENT_NONE				0x00000000
#define WTS_EVENT_CREATE			0x00000001
#define WTS_EVENT_DELETE			0x00000002
#define WTS_EVENT_RENAME			0x00000004
#define WTS_EVENT_CONNECT			0x00000008
#define WTS_EVENT_DISCONNECT		0x00000010
#define WTS_EVENT_LOGON				0x00000020
#define WTS_EVENT_LOGOFF			0x00000040
#define WTS_EVENT_STATECHANGE		0x00000080
#define WTS_EVENT_LICENSE			0x00000100
#define WTS_EVENT_ALL				0x7FFFFFFF
#define WTS_EVENT_FLUSH				0x80000000

BOOL WINAPI WTSQuerySessionInformationA(HANDLE hServer, DWORD SessionId, WTS_INFO_CLASS WTSInfoClass,
                                LPSTR *ppBuffer, DWORD *pBytesReturned);
BOOL WINAPI WTSQuerySessionInformationW(HANDLE hServer, DWORD SessionId, WTS_INFO_CLASS WTSInfoClass,
                                LPTSTR *ppBuffer, DWORD *pBytesReturned);
BOOL WINAPI WTSWaitSystemEvent(HANDLE hServer, DWORD EventMask, DWORD* pEventFlags);
BOOL WINAPI WTSDisconnectSession(HANDLE hServer, DWORD SessionId, BOOL bWait);
BOOL WINAPI WTSEnumerateSessionsW(HANDLE hServer, DWORD Reserved, DWORD Version,
				  PWTS_SESSION_INFOW *ppSessionInfo,
				  PDWORD pCount);
BOOL WINAPI WTSEnumerateSessionsA(HANDLE hServer, DWORD Reserved, DWORD Version,
				  PWTS_SESSION_INFOA *ppSessionInfo,
				  PDWORD pCount);
void WINAPI WTSFreeMemory(PVOID pMemory);

#ifdef UNICODE
#define WTSEnumerateSessions WTSEnumerateSessionsW
#define WTSQuerySessionInformation WTSQuerySessionInformationW
#else
#define WTSEnumerateSessions WTSEnumerateSessionsA
#define WTSQuerySessionInformation WTSQuerySessionInformationA
#endif

BOOL WTSSendMessageA(
  HANDLE hServer,
  DWORD SessionId,
  LPSTR pTitle,
  DWORD TitleLength,
  LPSTR pMessage,
  DWORD MessageLength,
  DWORD Style,
  DWORD Timeout,
  DWORD *pResponse,
  BOOL bWait
);

BOOL WTSSendMessageW(
  HANDLE hServer,
  DWORD SessionId,
  LPWSTR pTitle,
  DWORD TitleLength,
  LPWSTR pMessage,
  DWORD MessageLength,
  DWORD Style,
  DWORD Timeout,
  DWORD *pResponse,
  BOOL bWait
);

#ifdef UNICODE
#define WTSSendMessage WTSSendMessageW
#else
#define WTSSendMessage WTSSendMessageA
#endif

#endif /* _WIN32_WINNT >= 0x0500 */

#ifdef __cplusplus
}
#endif

#endif /* _WTSAPI32_H */
