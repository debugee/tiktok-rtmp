#include "charset.h"

unsigned char charset::toHex(const unsigned char &n)
{
    return n > 9 ? n - 10 + 'A' : n + '0';
}

unsigned char charset::fromHex(const unsigned char &c)
{
    return c >= '0' && c <= '9' ? c - '0' : c - 'A' + 10;
}

string charset::URLEncode(const string &sIn, bool spacePlus)
{
    string sOut;
    size_t size = sIn.size();
    for (size_t ix = 0; ix < size; ix++)
    {
        char buf[4] = {0};
        if (
            /*sIn[ix] == '~' ||*/
            sIn[ix] == '.' ||
            sIn[ix] == '-' ||
            sIn[ix] == '_' ||
            (sIn[ix] <= '9' && sIn[ix] >= '0') ||
            (sIn[ix] <= 'z' && sIn[ix] >= 'a') ||
            (sIn[ix] <= 'Z' && sIn[ix] >= 'A'))
        {
            buf[0] = sIn[ix];
        }
        else if (sIn[ix] == ' ' && spacePlus)
        {
            buf[0] = '+';
        }
        else
        {
            buf[0] = '%';
            buf[1] = (char)toHex((unsigned char)sIn[ix] >> 4);
            buf[2] = (char)toHex((unsigned char)sIn[ix] % 16);
        }
        sOut += buf;
    }
    return sOut;
};

string charset::URLDecode(const string &sIn)
{
    string sOut;
    size_t size = sOut.size();
    for (size_t ix = 0; ix < size; ix++)
    {
        char ch = 0;
        if (sIn[ix] == '%')
        {
            ch = (char)(fromHex((unsigned char)sIn[ix + 1]) << 4);
            ch |= fromHex((unsigned char)sIn[ix + 2]);
            ix += 2;
        }
        else if (sIn[ix] == '+')
        {
            ch = ' ';
        }
        else
        {
            ch = sIn[ix];
        }
        sOut += (char)ch;
    }
    return sOut;
}

size_t charset::u8Size(u8char u8Char)
{
    if ((u8Char & 0xFE) == 0xFC)
        return 6;

    if ((u8Char & 0xFC) == 0xF8)
        return 5;

    if ((u8Char & 0xF8) == 0xF0)
        return 4;

    if ((u8Char & 0xF0) == 0xE0)
        return 3;

    if ((u8Char & 0xE0) == 0xC0)
        return 2;

    if ((u8Char & 0x80) == 0x00)
        return 1;

    return 0;
}

size_t charset::u32ToU8(u32char u32In, u8char *u8Out)
{
    // sizeof u8Out >= 6
    uint32_t u32c = (uint32_t)u32In;
    uint8_t *u8c = (uint8_t *)u8Out;
    if (u32c <= 0x0000007F)
    {
        // * U-00000000 - U-0000007F:  0xxxxxxx
        *u8c = (u32c & 0x7F);
        return 1;
    }
    else if (u32c >= 0x00000080 && u32c <= 0x000007FF)
    {
        // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
        *(u8c + 1) = (u32c & 0x3F) | 0x80;
        *u8c = ((u32c >> 6) & 0x1F) | 0xC0;
        return 2;
    }
    else if (u32c >= 0x00000800 && u32c <= 0x0000FFFF)
    {
        // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
        *(u8c + 2) = (u32c & 0x3F) | 0x80;
        *(u8c + 1) = ((u32c >> 6) & 0x3F) | 0x80;
        *u8c = ((u32c >> 12) & 0x0F) | 0xE0;
        return 3;
    }
    else if (u32c >= 0x00010000 && u32c <= 0x001FFFFF)
    {
        // * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(u8c + 3) = (u32c & 0x3F) | 0x80;
        *(u8c + 2) = ((u32c >> 6) & 0x3F) | 0x80;
        *(u8c + 1) = ((u32c >> 12) & 0x3F) | 0x80;
        *u8c = ((u32c >> 18) & 0x07) | 0xF0;
        return 4;
    }
    else if (u32c >= 0x00200000 && u32c <= 0x03FFFFFF)
    {
        // * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(u8c + 4) = (u32c & 0x3F) | 0x80;
        *(u8c + 3) = ((u32c >> 6) & 0x3F) | 0x80;
        *(u8c + 2) = ((u32c >> 12) & 0x3F) | 0x80;
        *(u8c + 1) = ((u32c >> 18) & 0x3F) | 0x80;
        *u8c = ((u32c >> 24) & 0x03) | 0xF8;
        return 5;
    }
    else if (u32c >= 0x04000000 && u32c <= 0x7FFFFFFF)
    {
        // * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(u8c + 5) = (u32c & 0x3F) | 0x80;
        *(u8c + 4) = ((u32c >> 6) & 0x3F) | 0x80;
        *(u8c + 3) = ((u32c >> 12) & 0x3F) | 0x80;
        *(u8c + 2) = ((u32c >> 18) & 0x3F) | 0x80;
        *(u8c + 1) = ((u32c >> 24) & 0x3F) | 0x80;
        *u8c = ((u32c >> 30) & 0x01) | 0xFC;
        return 6;
    }

    return 0;
}

size_t charset::u8ToU32(const u8char *u8In, size_t inSz, u32char *u32Out)
{
    *u32Out = 0;
    uint8_t b1, b2, b3, b4, b5, b6;
    uint8_t *u8c = (uint8_t *)u8In;
    uint8_t *pOutput = (uint8_t *)u32Out;
    
    size_t size = 0;
    if (0 == inSz || (size = u8Size(*u8In)) > inSz)
    {
        return 0;
    }
    switch (size)
    {
    case 1:
        *pOutput = *u8c;
        break;
    case 2:
        b1 = *u8c;
        b2 = *(u8c + 1);
        if ((b2 & 0xC0) != 0x80)
            return 0;
        *pOutput = (b1 << 6 & 0xc0) + (b2 & 0x3F);
        *(pOutput + 1) = (b1 >> 2) & 0x07;
        break;
    case 3:
        b1 = *u8c;
        b2 = *(u8c + 1);
        b3 = *(u8c + 2);
        if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80))
            return 0;
        *pOutput = ((b2 << 6 & 0xc0) + (b3 & 0x3F));
        *(pOutput + 1) = ((b1 << 4 & 0xf0) + ((b2 >> 2) & 0x0F));
        break;
    case 4:
        b1 = *u8c;
        b2 = *(u8c + 1);
        b3 = *(u8c + 2);
        b4 = *(u8c + 3);
        if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) || ((b4 & 0xC0) != 0x80))
            return 0;
        *pOutput = ((b3 << 6 & 0xc0) + (b4 & 0x3F));
        *(pOutput + 1) = (b2 << 4 & 0xf0) + ((b3 >> 2) & 0x0F);
        *(pOutput + 2) = ((b1 << 2) & 0x1C) + ((b2 >> 4) & 0x03);
        break;
    case 5:
        b1 = *u8c;
        b2 = *(u8c + 1);
        b3 = *(u8c + 2);
        b4 = *(u8c + 3);
        b5 = *(u8c + 4);
        if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80))
            return 0;
        *pOutput = (b4 << 6 & 0xc0) + (b5 & 0x3F);
        *(pOutput + 1) = (b3 << 4 & 0xf0) + ((b4 >> 2) & 0x0F);
        *(pOutput + 2) = (b2 << 2 & 0xfc) + ((b3 >> 4) & 0x03);
        *(pOutput + 3) = (b1 & 0x03);
        break;
    case 6:
        b1 = *u8c;
        b2 = *(u8c + 1);
        b3 = *(u8c + 2);
        b4 = *(u8c + 3);
        b5 = *(u8c + 4);
        b6 = *(u8c + 5);
        if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80) || ((b6 & 0xC0) != 0x80))
            return 0;
        *pOutput = (b5 << 6 & 0xc0) + (b6 & 0x3F);
        *(pOutput + 1) = (b5 << 4 & 0xf0) + ((b6 >> 2) & 0x0F);
        *(pOutput + 2) = (b3 << 2 & 0xfc) + ((b4 >> 4) & 0x03);
        *(pOutput + 3) = ((b1 << 6) & 0x40) + (b2 & 0x3F);
        break;
    default:
        return 0;
    }

    return size;
}

bool charset::ucs2ToU8Str(const u16str &inUcs2Str, u8str &outU8Str)
{
    size_t sOne = 0;
    size_t size = inUcs2Str.size();
    size_t i = 0;
    for (; i < size; i++){
        u8char u8temp [6] = {0};
        u32char c = (uint16_t)inUcs2Str[i];
        sOne = u32ToU8(c, u8temp);
        if (sOne == 0)
            break;
        outU8Str.insert(outU8Str.end(), u8temp, u8temp + sOne);
    }
    return i == size ? true : false;
}

bool charset::u8ToUcs2Str(const u8str &inU8Str, u16str &outUcs2Str)
{
    size_t sOne = 0;
    size_t size = inU8Str.size();
    for (size_t pos = 0; size > 0; size -= sOne, pos += sOne)
    {
        uint32_t u32 = 0;
        sOne = u8ToU32(&inU8Str.at(pos), size, (u32char *)&u32);
        if (sOne < 1 || sOne > 3)
            break;
        outUcs2Str.push_back((u16char)u32);
    }
    return size == 0 ? true : false;
}

bool charset::u16ToU8Str(const u16str &inU16Str, u8str &outU8Str)
{
    size_t sOne = 0;
    size_t size = inU16Str.size();
    size_t i = 0;
    for (; i < size; i++){
        u8char u8temp [6] = {0};
        uint16_t w1 = (uint16_t)inU16Str[i];
        uint16_t w2 = 0;
        uint32_t c = 0;
        if (w1 >= 0xD800 && w1 <= 0xDBFF)
		{
			if (size - i <= 1)
			{
				return false;
			}
            w2 = (uint16_t)inU16Str[++i];
            if (w2 > 0xDFFF || w2 < 0xDC00)
                return false;

			c = (uint32_t)(((w1 & 0x3FF) << 10)|(w2 & 0x3FF));
			c += 0x10000;
		}
        else
        {
            c = w1;
        }
        sOne = u32ToU8((u32char)c, u8temp);
        if (sOne == 0)
            return false;
        outU8Str.insert(outU8Str.end(), u8temp, u8temp + sOne);
    }
    return i == size ? true : false;
}

bool charset::u8ToU16Str(const u8str &inU8Str, u16str &outU16Str)
{
    size_t sOne = 0;
    size_t size = inU8Str.size();
    for (size_t pos = 0; size > 0; size -= sOne, pos += sOne)
    {
        uint32_t c = 0;
        sOne = u8ToU32(&inU8Str.at(pos), size, (u32char *)&c);
        if (c >= 0x10000 && c <= 0x10FFFF && sOne == 4)
        {
 			c -= 0x10000;
 			u16char w1 = (u16char)((c >> 10) | 0xD800);
 			u16char w2 = (u16char)((c & 0x3FF) | 0xDC00);
            outU16Str.push_back(w1);
            outU16Str.push_back(w2);
        }
        else if (c <= 0xffff && sOne >= 1 && sOne <= 3)
        {
            outU16Str.push_back((u16char)c);
        }
        else
        {
            return false;
        }
    }
    return size == 0 ? true : false;
}

bool charset::u32ToU8Str(const u32str &inU32Str, u8str &outU8Str)
{
    size_t sOne = 0;
    size_t size = inU32Str.size();
    size_t i = 0;
    for (; i < size; i++){
        u8char u8temp [6] = {0};
        sOne = u32ToU8(inU32Str[i], u8temp);
        if (sOne == 0)
            break;
        outU8Str.insert(outU8Str.end(), u8temp, u8temp + sOne);
    }
    return i == size ? true : false;
}

bool charset::u8ToU32Str(const u8str &inU8Str, u32str &outU32Str)
{
    size_t sOne = 0;
    size_t size = inU8Str.size();
    for (size_t pos = 0; size > 0; size -= sOne, pos += sOne)
    {
        u32char u32 = 0;
        sOne = u8ToU32(&inU8Str.at(pos), size, &u32);
        if (sOne == 0)
            break;
        outU32Str.push_back(u32);
    }
    return size == 0 ? true : false;
}


charset::u32str charset::u8ToU32Str(const u8str &inU8Str)
{
	charset::u32str output;
	u8ToU32Str(inU8Str, output);
	return output;
}

charset::u8str  charset::u32ToU8Str(const u32str &inU32Str)
{
	charset::u8str output;
	u32ToU8Str(inU32Str, output);
	return output;
}

charset::u16str charset::u8ToU16Str(const u8str &inU8Str)
{
	charset::u16str output;
	u8ToU16Str(inU8Str, output);
	return output;
}

charset::u8str  charset::u16ToU8Str(const u16str &inU16Str)
{
	charset::u8str output;
	u16ToU8Str(inU16Str, output);
	return output;
}

charset::u8str  charset::ucs2ToU8Str(const u16str &inUcs2Str)
{
	u8str output;
	ucs2ToU8Str(inUcs2Str, output);
	return output;
}

charset::u16str charset::u8ToUcs2Str(const u8str &inU8Str)
{
	u16str output;
	u8ToUcs2Str(inU8Str, output);
	return output;
}