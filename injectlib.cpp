#include "injectlib.h"
#include "winprocess.h"
#include "decoder/bochs.h"
#include "decoder/instr.h"
#include "decoder/fetchdecode.h"
#include "decoder/decoder.h"

extern int fetchDecode32(const Bit8u *fetchPtr, bool is_32, bxInstruction_c *i, unsigned remainingInPage);
#if BX_SUPPORT_X86_64
extern int fetchDecode64(const Bit8u *fetchPtr, bxInstruction_c *i, unsigned remainingInPage);
#endif
extern const char *get_bx_opcode_name(Bit16u ia_opcode);


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

bool epInjectDll(const std::string &app, const std::string &paramters,
	const std::string &workDir, const std::string &dll)
{
	bool result = false;
#ifdef _WIN32
	CWinProcess process;
	unsigned flag = PAGE_EXECUTE_READWRITE;
#endif
	if (!process.start(app, workDir, paramters))
		return false;

	void *entryPoint = process.getEP();
	if (entryPoint == nullptr)
		return false;

	unsigned char jmpIndirect[6] = { 0xff, 0x25, 0x00, 0x00, 0x00, 0x00 };
	std::string opcode;
	std::string readBuffer;
	while (opcode.size() < 5)
	{
		readBuffer.resize(15);
		if (!process.readP((char *)entryPoint + opcode.size(), readBuffer))
			return false;

		bxInstruction_c instr = { 0 };
		int fetch = -1;
#if  defined(_WIN64) || defined(__x86_64__)
			fetch = fetchDecode64((const Bit8u *)readBuffer.data(), &instr, (unsigned)readBuffer.size());
#else
			fetch = fetchDecode32((const Bit8u *)readBuffer.data(), true, &instr, (unsigned)readBuffer.size());
#endif
		if (fetch != 0)
			return false;
		
		readBuffer.resize(instr.ilen());
		opcode += readBuffer;
	}

	void *middle = process.findMiddleSpace(sizeof(jmpIndirect) +
		sizeof(void *) + 
		opcode.size() + 
		sizeof(jmpIndirect) + 
		sizeof(void *));
	if (!middle)
		return nullptr;
	
	//re relative imm			
	intptr_t diff = (char *)middle +
				sizeof(jmpIndirect) + 
				sizeof(void *) - 
				(char *)entryPoint;
	opcode = reloc(opcode, diff);

	std::string shellcodeLoaddll = process.getShellCode(dll);
	char *newEP = (char *)middle + sizeof(jmpIndirect) + sizeof(void *);
	std::string shellcode;
	shellcode += shellcodeLoaddll;
	shellcode.append((const char *)jmpIndirect, sizeof(jmpIndirect));
	shellcode.append((char *)&newEP, sizeof(char *));

	void *jmpTo = process.allocP(shellcode.size(), flag);
	if (!jmpTo)
		return nullptr;
#if  !defined(_WIN64) && !defined(__x86_64__)
		*(void **)(shellcode.data() + shellcodeLoaddll.size() + 2) = (char *)jmpTo +
			shellcodeLoaddll.size() +
			sizeof(jmpIndirect);
#endif
	process.writeP(jmpTo, shellcode);

	std::string middleBuffer;
#if  !defined(_WIN64) && !defined(__x86_64__)
		*(void **)&jmpIndirect[2] = (char *)middle + 
			sizeof(jmpIndirect);
#endif
	middleBuffer.append((const char *)jmpIndirect, sizeof(jmpIndirect));
	middleBuffer.append((char *)&jmpTo, sizeof(void *));
	middleBuffer += opcode;
#if  !defined(_WIN64) && !defined(__x86_64__)
		*(void **)&jmpIndirect[2] = (char *)middle + 
			sizeof(jmpIndirect) + 
			sizeof(void *) + 
			opcode.size() + 
			sizeof(jmpIndirect);
#endif
	middleBuffer.append((const char *)jmpIndirect, sizeof(jmpIndirect));
	void *epBack = (char *)entryPoint + opcode.size();
	middleBuffer.append((char *)&epBack, sizeof(void *));
	process.writeP(middle, middleBuffer);
	process.protectP(middle, middleBuffer.size(), flag);

	diff = (char *)middle - ((char *)entryPoint + 5);
	std::string epBuffer;
	epBuffer.push_back((char)0xe9);
	int32_t diff32 = (int32_t)diff;
	epBuffer.append((char *)&diff32, sizeof(diff32));
	process.writeP(entryPoint, epBuffer);

	process.resume();

	return result;
}