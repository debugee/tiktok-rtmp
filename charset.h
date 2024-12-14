#ifndef __CHARSET_H__
#define __CHARSET_H__
#include <string>
#include <vector>
#include <stdint.h>
using namespace std;


namespace charset
{
	typedef char32_t u32char;
	typedef wchar_t u16char;
	typedef char u8char;
	typedef basic_string<u32char> u32str;
	typedef basic_string<u16char> u16str;
	typedef basic_string<u8char> u8str;

	bool ucs2ToU8Str(const u16str &inUcs2Str, u8str &outU8Str);
	bool u8ToUcs2Str(const u8str &inU8Str, u16str &outUcs2Str);
	bool u16ToU8Str(const u16str &inU16Str, u8str &outU8Str);
	bool u8ToU16Str(const u8str &inU8Str, u16str &outU16Str);
	bool u32ToU8Str(const u32str &inU32Str, u8str &outU8Str);
	bool u8ToU32Str(const u8str &inU8Str, u32str &outU32Str);
	u32str u8ToU32Str(const u8str &inU8Str);
	u8str  u32ToU8Str(const u32str &inU32Str);
	u16str u8ToU16Str(const u8str &inU8Str);
	u8str  u16ToU8Str(const u16str &inU16Str);
	u8str  ucs2ToU8Str(const u16str &inUcs2Str);
	u16str u8ToUcs2Str(const u8str &inU8Str);

	size_t u32ToU8(u32char u32In, u8char *u8Out);
	size_t u8ToU32(const u8char* u8In, size_t inSz, u32char *u32Out);
	unsigned char fromHex(const unsigned char &c);
	unsigned char toHex(const unsigned char &n);
	string URLEncode(const string &sIn, bool spacePlus = false);
	string URLDecode(const string &sIn);
	size_t u8Size(u8char u8Char);
}

#endif