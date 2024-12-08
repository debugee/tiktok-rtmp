#pragma once
#include <Windows.h>
#include <string>

typedef struct _HOOKINFO
{
	PVOID OldAddress;
	PVOID StubAddress;
	PVOID StubNotApiAddress;
	BOOL bAllocStub;
	BYTE  SavedCode[32];
	BYTE SavedOpCode[32];
}HOOK_INFO, *PHOOK_INFO;

PVOID GetNewAddress(PHOOK_INFO Info);
size_t stolenSz(void* address, size_t sz);
std::string reloc(const std::string &opcode, ptrdiff_t diff);
void* findMiddleSpace(void *pe, size_t sz);
BOOL HookByAddress(PVOID HookFunc, PHOOK_INFO Info, PVOID NewFunction, BOOL longJmp = FALSE, BOOL bStub = FALSE, BOOL bFuncMiddle = FALSE, unsigned char opcodes[20] = nullptr);
BOOL HookAPIByName(char *ModuleName, char *FuncName, PHOOK_INFO Info, PVOID NewFunction);
VOID UnHookAPI(PHOOK_INFO Info);
VOID UnHook(PHOOK_INFO Info);
