#pragma once

#include <cstdint>

using utf8 = char;

uint32_t utf8_get_next(const utf8* char_ptr, const utf8** nextchar_ptr);
utf8* utf8_write_codepoint(utf8* dst, uint32_t codepoint);
int32_t utf8_insert_codepoint(utf8* dst, uint32_t codepoint);
bool utf8_is_codepoint_start(const utf8* text);
void utf8_remove_format_codes(utf8* text, bool allowcolours);
int32_t utf8_get_codepoint_length(char32_t codepoint);
int32_t utf8_length(const utf8* text);
utf8* get_string_end(const utf8* text);
int32_t utf8_get_format_code_arg_length(char32_t codepoint);
bool utf8_is_format_code(char32_t codepoint);
bool utf8_is_colour_code(char32_t codepoint);
