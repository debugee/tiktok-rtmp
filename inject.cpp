#include <Windows.h>
#include <tchar.h>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>
#include "System.h"

using namespace std;

ULONG GetParentProcessID(ULONG dwId);

string g_p_BaseThreadInitThunk_Opcode;
string g_p_LdrLoadDll_Opcode;
string g_p_DbgUiRemoteBreakin_Opcode;
string g_p_SetUnhandledExceptionFilter_Opcode;


BOOL InjectDllByPid(DWORD dwPid, HMODULE hKernelRemote, HMODULE hNtdllRemote, wstring szdllName)
{
	BOOL bRet = FALSE;
	WCHAR szDllPath[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, szDllPath, MAX_PATH);
	PathRemoveFileSpecW(szDllPath);
	PathAppendW(szDllPath, szdllName.c_str());
	
	HANDLE hProcess = NULL;
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, dwPid);
	if (NULL == hProcess)
		return bRet;

	HMODULE modHandle = GetModuleHandle(_T("Kernel32.dll"));
	HMODULE modNtdll = GetModuleHandle(_T("ntdll.dll"));
	FARPROC lpOrigThreadThunk = GetProcAddress(modHandle, "BaseThreadInitThunk");
	void *lpRemoteThreadThunk = (void *)((DWORD_PTR)lpOrigThreadThunk - (DWORD_PTR)modHandle + (DWORD_PTR)hKernelRemote);
	bRet = WriteProcessMemory(hProcess, lpRemoteThreadThunk, g_p_BaseThreadInitThunk_Opcode.data(), g_p_BaseThreadInitThunk_Opcode.size(), NULL);//

	FARPROC lpOrigLdrLoadDll = GetProcAddress(modNtdll, "LdrLoadDll");
	void *lpRemoteLdrLoadDll = (void *)((DWORD_PTR)lpOrigLdrLoadDll - (DWORD_PTR)modNtdll + (DWORD_PTR)hNtdllRemote);
	bRet &= WriteProcessMemory(hProcess, lpRemoteLdrLoadDll, g_p_LdrLoadDll_Opcode.data(), g_p_LdrLoadDll_Opcode.size(), NULL);//

	void* pLibRemote = VirtualAllocEx(hProcess, NULL, MAX_PATH, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	LPTHREAD_START_ROUTINE pfnLoadLibraryLocal = (LPTHREAD_START_ROUTINE)GetProcAddress(modHandle, "LoadLibraryW");
	LPTHREAD_START_ROUTINE pfnLoadLibraryRemote = (LPTHREAD_START_ROUTINE)((DWORD_PTR)pfnLoadLibraryLocal - (DWORD_PTR)modHandle + (DWORD_PTR)hKernelRemote);

	bRet &= WriteProcessMemory(hProcess, pLibRemote, (void*)szDllPath, MAX_PATH, NULL);

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
		pfnLoadLibraryRemote,
		pLibRemote,
		0,
		NULL);

	if (hThread)
		CloseHandle(hThread);
	if (hProcess)
		CloseHandle(hProcess);
	return bRet && hThread;
}

BOOL tryInjectByPid(DWORD dwProcId)
{
	BOOL bFindDll = FALSE;
	HMODULE hKernel = NULL;
	HMODULE kNtdll = NULL;
	vector<MODULEENTRY32> * pModules = System::GetModulesByPid(dwProcId);
	if (pModules)
	{
		for (vector<MODULEENTRY32>::iterator iter = pModules->begin(); iter != pModules->end(); iter++)
		{
			tstring module_name = iter->szModule;
			std::transform(module_name.begin(), module_name.end(), module_name.begin(), ::tolower);
			if (module_name == _T("libtiktok.dll"))
			{
				bFindDll = TRUE;
			}
			if (module_name ==  _T("kernel32.dll"))
			{
				hKernel = iter->hModule;
			}
			if (module_name == (_T("ntdll.dll")))
			{
				kNtdll = iter->hModule;
			}
		}
		delete pModules;
	}
	if (!bFindDll && hKernel && kNtdll)
	{
		return InjectDllByPid(dwProcId, hKernel, kNtdll, L"libtiktok.dll");
	}

	return bFindDll;
}


BOOL InjectMediaSdk()
{
	vector<PROCESSENTRY32> *pProcList = System::GetProcList();
	if (pProcList)
	{
		for (vector<PROCESSENTRY32>::iterator iter = pProcList->begin(); iter != pProcList->end(); iter++)
		{
			tstring szExePath;
			System::GetProcPathByPid(iter->th32ProcessID, szExePath);
			if (std::string::npos != szExePath.find(_T("MediaSDK_Server.exe")))
			{
				return tryInjectByPid(iter->th32ProcessID);
			}
		}
		delete pProcList;
	}

	return FALSE;
}