#pragma once
#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <Shlwapi.h>
#include <vector>
#include <string>

#ifdef _UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif

using namespace std;

#pragma comment(lib, "psapi.lib")

namespace System
{
	inline BOOL GetRemoteModuleHandle(DWORD dwPid, LPCTSTR lpszModName, HMODULE &hModule)
	{
		BOOL bResult = FALSE;

		HANDLE hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);
		if (hSnap == INVALID_HANDLE_VALUE)
		{
			return bResult;
		}

		MODULEENTRY32 me32 = { 0 };
		me32.dwSize = sizeof(me32);
		if (::Module32First(hSnap, &me32))
		{
			do
			{
				if (0 == _tcscmp(lpszModName, me32.szModule))
				{
					hModule = me32.hModule;
					bResult = TRUE;
					break;
				}
			} while (::Module32Next(hSnap, &me32));
		}

		::CloseHandle(hSnap);

		return bResult;
	}

	inline HMODULE GetCurrentModule()
	{

		  MEMORY_BASIC_INFORMATION mbi;
		  static int dummy;
		  VirtualQuery( &dummy, &mbi, sizeof(mbi) );
	
		  return reinterpret_cast<HMODULE>(mbi.AllocationBase);
	}

	inline BOOL NtPathToDosPath(LPCTSTR pszNtPath, LPTSTR pszDosPath)
	{
		TCHAR            szDriveStr[500];
		TCHAR            szDrive[3];
		TCHAR            szDevName[100];
		INT                cchDevName;
		INT                i;

		if (!pszDosPath || !pszNtPath)
			return FALSE;

		if (!GetLogicalDriveStrings(sizeof(szDriveStr), szDriveStr))
			return FALSE;

		for (i = 0; szDriveStr[i]; i += 4)
		{
			//if (!lstrcmpi(&(szDriveStr[i]), _T("A:\\")) || !lstrcmpi(&(szDriveStr[i]), _T("B:\\")))
			//	continue;

			szDrive[0] = szDriveStr[i];
			szDrive[1] = szDriveStr[i + 1];
			szDrive[2] = '\0';
			if (!QueryDosDevice(szDrive, szDevName, 100))
				continue;

			cchDevName = lstrlen(szDevName);
			if (_tcsnicmp(pszNtPath, szDevName, cchDevName) == 0)
			{
				lstrcpy(pszDosPath, szDrive);
				lstrcat(pszDosPath, pszNtPath + cchDevName);
				return TRUE;
			}
		}

		return FALSE;
	}

	inline BOOL GetProcPathByPid(DWORD dwPid, tstring& strProcPath)
	{
		HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPid);
		if (NULL == hProcess)
			return FALSE;

		TCHAR modPath[MAX_PATH] = { 0 };
		DWORD nChrs = ::GetProcessImageFileName(hProcess, modPath, MAX_PATH);
		CloseHandle(hProcess);
		if (nChrs)
		{
			TCHAR dosPath[MAX_PATH] = { 0 };
			if (!NtPathToDosPath(modPath, dosPath))
			{
				return FALSE;
			}
			strProcPath = dosPath;
			return TRUE;
		}

		return FALSE;
	}

	inline vector<MODULEENTRY32> * GetModulesByPid(DWORD dwPid)
	{
		HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);
		if (INVALID_HANDLE_VALUE == hModuleSnap)
		{
			return NULL;
		}

		vector<MODULEENTRY32> * p = NULL;
		MODULEENTRY32 me32 = { 0 };
		me32.dwSize = sizeof(MODULEENTRY32);
		if (Module32First(hModuleSnap, &me32))
		{
			p = new vector<MODULEENTRY32>;
			do
			{
				p->push_back(me32);
			} while (Module32Next(hModuleSnap, &me32));
		}
		CloseHandle(hModuleSnap);
		return p;
	}

	inline vector<PROCESSENTRY32> *GetProcList()
	{
		HANDLE hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (INVALID_HANDLE_VALUE == hProcSnap)
		{
			return NULL;
		}

		vector<PROCESSENTRY32> * p = NULL;
		PROCESSENTRY32 pe32 = { 0 };
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hProcSnap, &pe32))
		{
			p = new vector<PROCESSENTRY32>;
			do
			{
				p->push_back(pe32);
			} while (Process32Next(hProcSnap, &pe32));
		}
		CloseHandle(hProcSnap);
		return p;
	}
}
