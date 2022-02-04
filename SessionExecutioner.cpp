#include <Windows.h>
#include <wtsapi32.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <tchar.h>
#include <string.h>
#pragma comment(lib, "wtsapi32.lib")


BOOL SetPrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	HANDLE hToken, hProcess;
	hProcess = GetCurrentProcess();
	OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken);

	if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
	{
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;


	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
	{
		return FALSE;
	}

	return TRUE;

}


DWORD GetActiveSession()
{
	WTS_SESSION_INFO* WTSSessionInfo = new WTS_SESSION_INFO[15];
	DWORD dwCount, dwState, dwSessionId;

	/*
		Iterating over existing sessions to find the active one.
	*/
	WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &WTSSessionInfo, &dwCount);
	for (size_t i = 0; i < dwCount; i++)
	{
		dwState = WTSSessionInfo[i].State;
		dwSessionId = WTSSessionInfo[i].SessionId;
		if (dwState == 0)
		{
			WTSFreeMemory(WTSSessionInfo);
			return dwSessionId;
		}
	}
	WTSFreeMemory(WTSSessionInfo);
	return 0;


}

DWORD GetProcessByActiveSession()
{
	DWORD dwCurrentSession = GetActiveSession();

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 hProcess;
	DWORD dwSessionId;
	ZeroMemory(&hProcess, sizeof(hProcess));
	hProcess.dwSize = sizeof(hProcess);

	/*
		Iterating over existing processes until finding the process "explorer.exe" that belongs to the current Active Session,
		then returns its process ID.
	*/
	if (Process32First(hSnapshot, &hProcess))
	{
		do
		{
			BOOL bSuccess = ProcessIdToSessionId(hProcess.th32ProcessID, &dwSessionId);
			if (dwSessionId == dwCurrentSession && _tcscmp(hProcess.szExeFile, TEXT("explorer.exe")) == 0)
			{
				return hProcess.th32ProcessID;
			}
		} while (Process32Next(hSnapshot, &hProcess));
	}

	return 0;
}

HANDLE AcquireTokenOfProcessInsideSession()
{

	DWORD dwProcessId = 0;
	HANDLE hProcess, hToken = NULL, hUserTokenDup = NULL;

	SECURITY_ATTRIBUTES sa;
	memset(&sa, 0, sizeof(sa));
	sa.nLength = sizeof(sa);

	/*
		Trying to get the PID of the current active session's "explorer.exe"
	*/
	while ((dwProcessId = GetProcessByActiveSession()) == 0)
	{
		Sleep(1000);
	}

	hProcess = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
	if (hProcess == NULL)
	{
		return NULL;
	}

	/*
		Open and duplicate the token, then return it.
	*/
	if (!OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken))
	{
		CloseHandle(hProcess);
		return NULL;
	}

	if (!DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, &sa, SecurityImpersonation, TokenPrimary, &hUserTokenDup))
	{
		CloseHandle(hToken);
		CloseHandle(hProcess);
		return NULL;
	}


	return hUserTokenDup;
}


DWORD CreateProcessInsideActiveSession(LPCTSTR lpFilePath)
{
	HANDLE hToken = AcquireTokenOfProcessInsideSession();

	if (hToken)
	{
		STARTUPINFO sa = { 0 };
		PROCESS_INFORMATION pa = { 0 };

		CreateProcessAsUserW(hToken, lpFilePath, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &sa, &pa);
		return 0;
	}
	else
	{
		return 1;
	}

}



int main()
{
	int dwArgs = 0;
	LPWSTR* lpArgs;

	SetPrivilege(SE_ASSIGNPRIMARYTOKEN_NAME, TRUE);
	SetPrivilege(SE_DEBUG_NAME, TRUE);
	SetPrivilege(SE_INCREASE_QUOTA_NAME, TRUE);

	lpArgs = CommandLineToArgvW(GetCommandLineW(), &dwArgs);
	if (dwArgs == 2)
	{
		return CreateProcessInsideActiveSession(lpArgs[1]);
	}
	else
	{
		return 1;
	}
}
