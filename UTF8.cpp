/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "UTF8.h"

#include <wchar.h>
#include <cstring>

uint32_t utf8_get_next(const utf8* char_ptr, const utf8** nextchar_ptr)
{
    int32_t result;
    int32_t numBytes;

    if (!(char_ptr[0] & 0x80))
    {
        result = char_ptr[0];
        numBytes = 1;
    }
    else if ((char_ptr[0] & 0xE0) == 0xC0)
    {
        result = ((char_ptr[0] & 0x1F) << 6) | (char_ptr[1] & 0x3F);
        numBytes = 2;
    }
    else if ((char_ptr[0] & 0xF0) == 0xE0)
    {
        result = ((char_ptr[0] & 0x0F) << 12) | ((char_ptr[1] & 0x3F) << 6) | (char_ptr[2] & 0x3F);
        numBytes = 3;
    }
    else if ((char_ptr[0] & 0xF8) == 0xF0)
    {
        result = ((char_ptr[0] & 0x07) << 18) | ((char_ptr[1] & 0x3F) << 12) | ((char_ptr[2] & 0x3F) << 6)
            | (char_ptr[3] & 0x3F);
        numBytes = 4;
    }
    else
    {
        // TODO 4 bytes
        result = ' ';
        numBytes = 1;
    }

    if (nextchar_ptr != nullptr)
        *nextchar_ptr = char_ptr + numBytes;
    return result;
}

utf8* utf8_write_codepoint(utf8* dst, uint32_t codepoint)
{
    if (codepoint <= 0x7F)
    {
        dst[0] = (utf8)codepoint;
        return dst + 1;
    }
    else if (codepoint <= 0x7FF)
    {
        dst[0] = 0xC0 | ((codepoint >> 6) & 0x1F);
        dst[1] = 0x80 | (codepoint & 0x3F);
        return dst + 2;
    }
    else if (codepoint <= 0xFFFF)
    {
        dst[0] = 0xE0 | ((codepoint >> 12) & 0x0F);
        dst[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        dst[2] = 0x80 | (codepoint & 0x3F);
        return dst + 3;
    }
    else
    {
        dst[0] = 0xF0 | ((codepoint >> 18) & 0x07);
        dst[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        dst[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        dst[3] = 0x80 | (codepoint & 0x3F);
        return dst + 4;
    }
}

/**
 * Inserts the given codepoint at the given address, shifting all characters after along.
 * @returns the size of the inserted codepoint.
 */
int32_t utf8_insert_codepoint(utf8* dst, uint32_t codepoint)
{
    int32_t shift = utf8_get_codepoint_length(codepoint);
    utf8* endPoint = get_string_end(dst);
    memmove(dst + shift, dst, endPoint - dst + 1);
    utf8_write_codepoint(dst, codepoint);
    return shift;
}

bool utf8_is_codepoint_start(const utf8* text)
{
    if ((text[0] & 0x80) == 0)
        return true;
    if ((text[0] & 0xC0) == 0xC0)
        return true;
    return false;
}

int32_t utf8_get_codepoint_length(char32_t codepoint)
{
    if (codepoint <= 0x7F)
    {
        return 1;
    }
    else if (codepoint <= 0x7FF)
    {
        return 2;
    }
    else if (codepoint <= 0xFFFF)
    {
        return 3;
    }
    else
    {
        return 4;
    }
}

/**
 * Gets the number of characters / codepoints in a UTF-8 string (not necessarily 1:1 with bytes and not including null
 * terminator).
 */
int32_t utf8_length(const utf8* text)
{
    const utf8* ch = text;

    int32_t count = 0;
    while (utf8_get_next(ch, &ch) != 0)
    {
        count++;
    }
    return count;
}

/**
 * Returns a pointer to the null terminator of the given UTF-8 string.
 */
utf8* get_string_end(const utf8* text)
{
    int32_t codepoint;
    const utf8* ch = text;

    while ((codepoint = utf8_get_next(ch, &ch)) != 0)
    {
        int32_t argLength = utf8_get_format_code_arg_length(codepoint);
        ch += argLength;
    }
    return (utf8*)(ch - 1);
}

/**
 * Return the number of bytes (including the null terminator) in the given UTF-8 string.
 */
size_t get_string_size(const utf8* text)
{
    return get_string_end(text) - text + 1;
}

/**
 * Return the number of visible characters (excludes format codes) in the given UTF-8 string.
 */
int32_t get_string_length(const utf8* text)
{
    char32_t codepoint;
    const utf8* ch = text;

    int32_t count = 0;
    while ((codepoint = utf8_get_next(ch, &ch)) != 0)
    {
        if (utf8_is_format_code(codepoint))
        {
            ch += utf8_get_format_code_arg_length(codepoint);
        }
        else
        {
            count++;
        }
    }
    return count;
}

int32_t utf8_get_format_code_arg_length(char32_t codepoint)
{
    switch (codepoint)
    {
        case 3:
        case 4:
            return 1;
        default:
            return 0;
    }
}

void utf8_remove_formatting(utf8* string, bool allowColours)
{
    utf8* readPtr = string;
    utf8* writePtr = string;

    while (true)
    {
        char32_t code = utf8_get_next(readPtr, (const utf8**)&readPtr);

        if (code == 0)
        {
            *writePtr = 0;
            break;
        }
        else if (!utf8_is_format_code(code) || (allowColours && utf8_is_colour_code(code)))
        {
            writePtr = utf8_write_codepoint(writePtr, code);
        }
    }
}

bool utf8_is_format_code(char32_t codepoint)
{
    return false;
}

bool utf8_is_colour_code(char32_t codepoint)
{
    return false;
}
