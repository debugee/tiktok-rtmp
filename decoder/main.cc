#include "bochs.h"
#include "instr.h"

extern int fetchDecode32(const Bit8u *fetchPtr, bool is_32, bxInstruction_c *i, unsigned remainingInPage);
#if BX_SUPPORT_X86_64
extern int fetchDecode64(const Bit8u *fetchPtr, bxInstruction_c *i, unsigned remainingInPage);
#endif


#include <Windows.h>
#define ALIGN_UP(l, a) (((l + a - 1)/a)*a)

void* findOp(unsigned char* ptr, size_t size, size_t nOp, unsigned char op) {
	for (size_t i = 0; i < size; i++) {
		size_t m = 0;
		for (; m < nOp && i + m < size; m++) {
			if (ptr[i + m] != op)
				break;
		}
		if (m == nOp) {
			return ptr + i;
		}
	}
	return nullptr;
}

void* findGap(HMODULE hMod, int nOp) {
	PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)hMod;
	if ((WORD)'ZM' != pDosHdr->e_magic)
		return nullptr;
	PIMAGE_NT_HEADERS pNtHdr = (PIMAGE_NT_HEADERS)((ULONG_PTR)pDosHdr + pDosHdr->e_lfanew);
	if ((DWORD)'EP' != pNtHdr->Signature)
		return nullptr;
	PIMAGE_SECTION_HEADER pSectionHdr = IMAGE_FIRST_SECTION(pNtHdr);
	for (DWORD i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++) 
	{
		if (pSectionHdr[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)
		{
			DWORD dwVirtualSize = ALIGN_UP(pSectionHdr[i].Misc.VirtualSize, pNtHdr->OptionalHeader.SectionAlignment);
			dwVirtualSize = dwVirtualSize ? dwVirtualSize : pNtHdr->OptionalHeader.SectionAlignment;
			return findOp((unsigned char *)hMod + pSectionHdr[i].VirtualAddress + pSectionHdr[i].Misc.VirtualSize, dwVirtualSize - pSectionHdr[i].Misc.VirtualSize, nOp, 0);
		}
	}
	return nullptr;
}

void* findCC(unsigned char* ptr, size_t size, int nOp) {
	return findOp(ptr, size, nOp, 0xcc);
}

typedef struct _HOOKINFO
{
	PVOID OldAddress;
	PVOID StubAddress;
	PVOID CallAddress;
	BYTE  SavedCode[16];
	BYTE SavedOpCode[16];
}HOOK_INFO, *PHOOK_INFO;

int GetFunctionLen(BYTE* Address)
{
	int nTotalLen = 0;
	while (true) {
		bxInstruction_c i;
#ifdef _WIN64
	int ret = fetchDecode64(Address + nTotalLen, &i, 15);
#else
	int ret = fetchDecode32(Address + nTotalLen, 1, &i, 15);
#endif
		if (ret != 0)
			return -1;
		nTotalLen += i.ilen();
		if (nTotalLen >= 5)
			return nTotalLen;
	}
}

BOOL HookByAddress(PVOID HookFunc, PHOOK_INFO Info, PVOID NewFunction)
{
	int nOpcodesLen = GetFunctionLen((BYTE*)HookFunc);
	if (nOpcodesLen < 0)
		return FALSE;

	MEMORY_BASIC_INFORMATION mbi;
	if (::VirtualQuery(HookFunc, &mbi, sizeof(mbi)) == 0)
		return FALSE;

	Info->OldAddress = HookFunc;
	memcpy(Info->SavedCode, HookFunc, sizeof(Info->SavedCode));
	memcpy(Info->SavedOpCode, HookFunc, sizeof(Info->SavedOpCode));
	if (mbi.AllocationBase)//module
	{
		Info->StubAddress = findCC((unsigned char *)mbi.BaseAddress, mbi.RegionSize, (sizeof(LPVOID) + 6) * 2 + nOpcodesLen);
		if (!Info->StubAddress)
		{
			Info->StubAddress = findGap((HMODULE)mbi.AllocationBase, (sizeof(LPVOID) + 6) * 2 + nOpcodesLen);
		}
		if (!Info->StubAddress)
			return FALSE;
	}
	else
	{
		Info->StubAddress = findCC((unsigned char *)mbi.BaseAddress, mbi.RegionSize, (sizeof(LPVOID) + 6) * 2 + nOpcodesLen);
		if (!Info->StubAddress)
			return FALSE;
	}
	ULONG_PTR dwDisp = (ULONG_PTR)Info->StubAddress + sizeof(LPVOID) - (ULONG_PTR)Info->OldAddress - 5;
	if ((LONG_PTR)dwDisp > UINT_MAX)
		return FALSE;
	
	Info->SavedOpCode[0] = 0xe9;
	*(LPDWORD)&Info->SavedOpCode[1] = (DWORD)dwDisp;
	Info->CallAddress = NewFunction;
	if (!Info->CallAddress)
		return FALSE;	
	//1122334455667788
	//FF15 f2ffffff 
	//opcodes
	//FF25 00000000 
	//1122334455667788
	//cc
	BYTE Buffer[(sizeof(LPVOID) + 6) * 2 + 15 + 1] = { 0 };
	BYTE Address1[6] = { 0xff, 0x15, 0xf2, 0xff, 0xff, 0xff };
	BYTE Address2[6] = { 0xff, 0x25, 0x00, 0x00, 0x00, 0x00 };
#ifndef _WIN64
	*(LPDWORD)&Address1[2] = (DWORD)Info->StubAddress;
	*(LPDWORD)&Address2[2] = (DWORD)((BYTE *)Info->StubAddress + sizeof(LPVOID) + 6 + nOpcodesLen + 6);
#endif
	*(LPVOID*)&Buffer = Info->CallAddress;
	memcpy(&Buffer[sizeof(LPVOID)], Address1, 6);
	memcpy(&Buffer[sizeof(LPVOID) + 6], Info->SavedCode, nOpcodesLen);
	memcpy(&Buffer[sizeof(LPVOID) + 6 + nOpcodesLen], Address2, 6);
	LPVOID pRetBack = (BYTE *)Info->OldAddress + nOpcodesLen;
	memcpy(&Buffer[sizeof(LPVOID) + 6 + nOpcodesLen + 6], &pRetBack, sizeof(LPVOID));
	Buffer[sizeof(LPVOID) + 6 + nOpcodesLen + 6 + sizeof(LPVOID)] = 0xcc;

	DWORD StubPro = 0, OldPro = 0;
	VirtualProtect(Info->StubAddress, (sizeof(LPVOID) + 6) * 2 + nOpcodesLen + 1, PAGE_EXECUTE_READWRITE, &StubPro);
	memcpy(Info->StubAddress, Buffer, (sizeof(LPVOID) + 6) * 2 + nOpcodesLen + 1);
	VirtualProtect(Info->StubAddress, (sizeof(LPVOID) + 6) * 2 + nOpcodesLen + 1, StubPro, &StubPro);
	VirtualProtect(Info->OldAddress, 5, PAGE_EXECUTE_READWRITE, &OldPro);
	memcpy(Info->OldAddress, Info->SavedOpCode, 5);
	VirtualProtect(Info->OldAddress, 5, OldPro, &OldPro);
	return TRUE;
}

int fuckyou()
{
	int a = 123;
	int b = 123123;
	return a + b;
}

extern "C"
void proxy_xxx()
{

}

extern "C" void asm_xxx();
int test()
{
	HOOK_INFO hkinfo = { 0 };
	HookByAddress(&fuckyou, &hkinfo, asm_xxx);
	fuckyou();
	return 0;
}