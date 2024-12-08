#include "HookLib.h"
#include <string>
#include "decoder/bochs.h"
#include "decoder/instr.h"
#include "decoder/fetchdecode.h"
#include "decoder/decoder.h"

extern int fetchDecode32(const Bit8u *fetchPtr, bool is_32, bxInstruction_c *i, unsigned remainingInPage);
#if BX_SUPPORT_X86_64
extern int fetchDecode64(const Bit8u *fetchPtr, bxInstruction_c *i, unsigned remainingInPage);
#endif

#define PAGE_SIZE 0x1000

HMODULE ModuleFromAddress(PVOID pv)
{
	MEMORY_BASIC_INFORMATION mbi;
	if (::VirtualQuery(pv, &mbi, sizeof(mbi)) != 0)
	{
		return (HMODULE)mbi.AllocationBase;
	}
	else
	{
		return NULL;
	}
}

PVOID GetNewAddress(PHOOK_INFO Info)
{
	return (LPVOID)((LPBYTE)Info->StubAddress + sizeof(LPVOID) + 6);
}

size_t stolenSz(void* address, size_t sz)
{
	size_t total = 0;
	while (true) {
		bxInstruction_c i = { 0 };
#ifdef _WIN64
		int ret = fetchDecode64((const Bit8u *)address + total, &i, 15);
#else
		int ret = fetchDecode32((const Bit8u *)address + total, 1, &i, 15);
#endif
		if (ret != 0)
			return -1;
		total += i.ilen();
		if (total >= sz)
			return total;
	}
}

std::string reloc(const std::string &opcode, ptrdiff_t diff)
{
	std::string readBuffer;
	std::string opcodeCpy = opcode;

	while (opcodeCpy.size() > 0)
	{
		bxInstruction_c instr = { 0 };
		int fetch = -1;
#if  defined(_WIN64) || defined(__x86_64__)
		fetch = fetchDecode64((const Bit8u *)opcodeCpy.data(), &instr, (unsigned)opcodeCpy.size());
#else
		fetch = fetchDecode32((const Bit8u *)opcodeCpy.data(), true, &instr, (unsigned)opcodeCpy.size());
#endif
		if (fetch != 0)
			return false;

		extern bxIAOpcodeTable BxOpcodesTable[];
		Bit16u ia_opcode = instr.getIaOpcode();
		unsigned type_src = (unsigned)BxOpcodesTable[ia_opcode].src[0];
		unsigned type = BX_DISASM_SRC_TYPE(type_src);
		unsigned src = BX_DISASM_SRC_ORIGIN(type_src);
		//jxx imm32, call imm32
		if (type_src == OP_Jd || type_src == OP_Jq)
		{
			Bit32u newImm = (Bit32u)((Bit32s)instr.Id() - diff);
			readBuffer.append(opcodeCpy.data(), instr.ilen() - sizeof(Bit32u));
			readBuffer.append((const char *)&newImm, sizeof(Bit32u));
		}
		//rip relative, no support
		//else if (is64 && instr.sibBase() == BX_GENERAL_REGISTERS)//BX_64BIT_REG_RIP
		//{
		//	//...
		//}
		else
		{
			readBuffer.append(opcodeCpy.data(), instr.ilen());
		}

		opcodeCpy = opcodeCpy.substr(instr.ilen());
	}
	return readBuffer;
}

void* findMiddleSpace(void *pe, size_t sz)
{
	const PIMAGE_DOS_HEADER pDos = (const PIMAGE_DOS_HEADER)pe;
	const PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)((intptr_t)pe + pDos->e_lfanew);

#define ALIGN_UP(l, a) (((l + a - 1)/a)*a)
	DWORD dwOffset = pNt->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER) + sizeof(IMAGE_NT_HEADERS) + pDos->e_lfanew;
	dwOffset = (dwOffset + 15) & 0xfffffff0;
	DWORD dwSize = ALIGN_UP(pNt->OptionalHeader.SizeOfHeaders, pNt->OptionalHeader.SectionAlignment);

	char *address = (char *)pe + dwOffset;
	size_t nLen = dwSize - dwOffset;
	std::string buffer(address, nLen);
	std::string jmp(sz, 0);
	size_t pos = buffer.find(jmp);
	if (pos == std::string::npos)
		return nullptr;

	return (void *)((intptr_t)address + pos);
}

BOOL HookByAddress(PVOID HookFunc, PHOOK_INFO Info, PVOID NewFunction, BOOL longJmp, BOOL bStub, BOOL bFuncMiddle, unsigned char opcodes[20])
{
	unsigned char opcodex64[] = {
		0xEB, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x51, 0x52, 0x41, 0x50, 0x41,
		0x51, 0x41, 0x52, 0x41, 0x53, 0x9C, 0x48, 0x83, 0xEC, 0x20, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
		0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xFF, 0x15,
		0xCE, 0xFF, 0xFF, 0xFF, 0x48, 0x83, 0xC4, 0x20, 0x9D, 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41,
		0x58, 0x5A, 0x59, 0x58, 0xC3
	};
	unsigned char opcodex86[] = { 
		0x9C, 0x60,
		0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 
		0x90, 0x90, 0x90, 0x90, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x61, 0x9D, 0xC3};

	size_t stolenSzMini = 5;
#ifdef _WIN64
	stolenSzMini = longJmp ? 14 : 5;
#else
	longJmp = TRUE;
#endif

	Info->bAllocStub = longJmp;
	size_t nStolen = stolenSz(HookFunc, stolenSzMini);
	if (nStolen < 0)
		return FALSE;

	MEMORY_BASIC_INFORMATION mbi;
	if (::VirtualQuery(HookFunc, &mbi, sizeof(mbi)) == 0)
		return FALSE;
	if (bStub)
		Info->StubNotApiAddress = VirtualAlloc(NULL, PAGE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	Info->OldAddress = HookFunc;
	memcpy(Info->SavedCode, HookFunc, sizeof(Info->SavedCode));
	memcpy(Info->SavedOpCode, HookFunc, sizeof(Info->SavedOpCode));

	size_t middleSz = (sizeof(void *) + 6) * 2 + nStolen;
	if (longJmp)
	{
		Info->StubAddress = VirtualAlloc(NULL, PAGE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	}
	else
	{
		if (mbi.AllocationBase)//module
		{
			Info->StubAddress = findMiddleSpace(mbi.AllocationBase, middleSz + 16);
			if (Info->StubAddress)
				Info->StubAddress = (char *)Info->StubAddress + 16;
		}
	}
	if (!Info->StubAddress)
		return FALSE;

	if (stolenSzMini == 14)
	{
		//FF25 00000000 F0DEBC9A78563412
		Info->SavedOpCode[0] = 0xff;
		Info->SavedOpCode[1] = 0x25;
		Info->SavedOpCode[2] = 0x00;
		Info->SavedOpCode[3] = 0x00;
		Info->SavedOpCode[4] = 0x00;
		Info->SavedOpCode[5] = 0x00;
		*(void **)&Info->SavedOpCode[6] = (char *)Info->StubAddress + sizeof(void *);
	}
	else
	{
		ptrdiff_t distance = (intptr_t)Info->StubAddress + sizeof(void *) - (intptr_t)Info->OldAddress - 5;
		if (distance > UINT_MAX || -distance > UINT_MAX)
			return FALSE;
		//e9 00000000
		Info->SavedOpCode[0] = 0xe9;
		*(uint32_t *)&Info->SavedOpCode[1] = (uint32_t)distance;
	}
	//1122334455667788
	//FF25 f2ffffff 
	//opcodes
	//FF25 00000000 
	//1122334455667788
	//cc
	std::string middleBuffer;
	unsigned char Address1[6] = { 0xff, 0x25, 0xf2, 0xff, 0xff, 0xff };
	unsigned char Address2[6] = { 0xff, 0x25, 0x00, 0x00, 0x00, 0x00 };
#ifndef _WIN64
	*(void**)&Address1[2] = Info->StubAddress;
	*(void**)&Address2[2] = (char *)Info->StubAddress + sizeof(void *) + 6 + nStolen + 6;
#endif
	if (bStub){
		Address1[1] = 0x15;//call
		middleBuffer.append((char *)&Info->StubNotApiAddress, sizeof(void*));
#ifdef _WIN64
		*(void**)(opcodex64 + 2) = NewFunction;
		if (opcodes)
			memcpy(opcodex64 + 0x1A, opcodes, 20);
		if (bFuncMiddle)
		{
			*(unsigned char *)(opcodex64 + 0x19) = 0x28;
			*(unsigned char *)(opcodex64 + 0x37) = 0x28;
		}
		memcpy(Info->StubNotApiAddress, opcodex64, sizeof(opcodex64));
#else
		if (opcodes)
			memcpy(opcodex86 + 2, opcodes, 20);
		*(void **)&opcodex86[23] = (void*)((intptr_t)NewFunction - ((intptr_t)Info->StubNotApiAddress + 23 + sizeof(void *)));
		memcpy(Info->StubNotApiAddress, opcodex86, sizeof(opcodex86));
#endif		
	}else
	{
		middleBuffer.append((char *)&NewFunction, sizeof(void*));
	}
	middleBuffer.append((char *)Address1, sizeof(Address1));

	std::string stolenOp((char *)Info->SavedCode, nStolen);
	ptrdiff_t diff = (intptr_t)Info->StubAddress + sizeof(void *) + 6 - (intptr_t)Info->OldAddress;
	stolenOp = reloc(stolenOp, diff);
	middleBuffer += stolenOp;
	middleBuffer.append((char *)Address2, sizeof(Address2));

	void* retback = (char *)Info->OldAddress + nStolen;
	middleBuffer.append((char*)&retback, sizeof(void *));
	middleBuffer.push_back('\xcc');

	DWORD StubPro = 0, OldPro = 0;
	VirtualProtect(Info->StubAddress, middleBuffer.size(), PAGE_EXECUTE_READWRITE, &StubPro);
	memcpy(Info->StubAddress, middleBuffer.data(), middleBuffer.size());
	VirtualProtect(Info->StubAddress, middleBuffer.size(), StubPro | PAGE_EXECUTE_READ, &StubPro);
	VirtualProtect(Info->OldAddress, stolenSzMini, PAGE_EXECUTE_READWRITE, &OldPro);
	memcpy(Info->OldAddress, Info->SavedOpCode, stolenSzMini);
	VirtualProtect(Info->OldAddress, stolenSzMini, OldPro, &OldPro);
	return TRUE;
}

BOOL HookAPIByName(char *ModuleName, char *FuncName, PHOOK_INFO Info, PVOID NewFunction)
{
	PVOID FuncAddr;
	HMODULE hModule = GetModuleHandleA(ModuleName);
	if (hModule == NULL)
		hModule = LoadLibraryA(ModuleName);
	if (hModule == 0)
		return FALSE;
	FuncAddr = (PVOID)GetProcAddress(hModule, FuncName);
	if (FuncAddr == 0)
		return FALSE;
	return HookByAddress(FuncAddr, Info, NewFunction);
}

VOID UnHookAPI(PHOOK_INFO Info)
{
	DWORD OldPro;
	if (!Info)
		return;
	if (VirtualProtect(Info->OldAddress, sizeof(Info->SavedCode), PAGE_EXECUTE_READWRITE, &OldPro))
	{
		RtlCopyMemory(Info->OldAddress, Info->SavedCode, sizeof(Info->SavedCode));
		VirtualProtect(Info->OldAddress, sizeof(Info->SavedCode), OldPro, &OldPro);		
	}
	if (Info->bAllocStub)
		VirtualFree(Info->StubAddress, 0, MEM_RELEASE);
	if (Info->StubNotApiAddress)
		VirtualFree(Info->StubNotApiAddress, 0, MEM_RELEASE);
}

VOID UnHook(PHOOK_INFO Info)
{
	UnHookAPI(Info);
}