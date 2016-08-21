/*
This function splits a command string into an argv array. Quoting and escaping
are expected to be bash-style.

A bash commandline consists of the following things:
- strong quoted string (single quoted string - apostrophes)
- weak quoted string (double quoted string - quotation marks)
- unquoted string
- separators between strings (space/tab/newline)

Concatenating different types of quoted strings without separators (spaces/tabs)
between them is valid. This way we get a single string that appears as a single
word in the argv array. Eg.: 01'23'"45" is the concatention of 01, '23' and "45"
strings and it appears as a single 012345 string in the unquoted argv array.

An apostrophe behaves as a normal character in a weak quoted string and the
same is true about quotation marks inside strong quoted strings.

A strong quoted string consists of two apostrophes and the characters
of the string between them. The string can't contain the apostrophe character
itself but it can contain any other character (including \ and ") without
interpreting them specially.

The characters of unquoted and weak quoted strings can be escaped.
You can escape ANY character by simply prefixing it with a backslash. The
unescaped result is the escaped character itself without interpreting it
specially (like the C compiler with \n, \t, ...).

Escaping is useful in the following scenarios:
- You can place a quotation mark inside a inside a weak quoted
  string by escaping it. You can escape any other character inside
  a weak quoted string but it is totally unnecessary.
- Outside of quoted strings you can escape apostrophes and quotation
  marks to force them to behave as normal characters that don't
  start new quoted strings.
- Escaping a space/tab/newline character outside of quoted strings
  prevents it from behaving as a separator character.
  E.g.: "single argv word" is equivalent to single\ argv\ word
*/
#pragma once

#include <stddef.h>


typedef enum {
	C2A_OK,
	C2A_BUF_TOO_SMALL,   // *buf_len isn't large enough
	C2A_ARGV_TOO_SMALL,  // *argv_len isn't large enough
	C2A_UNMATCHED_QUOT,  // reached the end of cmd while looking for matching ' or "
	C2A_LONELY_ESCAPE,   // cmd ends with \ but the escaped character is missing
} C2A_RESULT;

// cmd2argv splits a bash-style command into an argv array.
//
// cmd:
//		Points to the zero terminated string that represents the command.
//
// buf:
//		Points to a buffer where this function will store the unescaped/unqouted
//		strings of the resulting argv array. On entry the size of this buffer
//		should be *buf_len. The required buffer size is never more than
//		the lenght of the command string + 1 (because of the terminating zero)
//		in the cmd parameter.
//		Note that buf can point to the same buffer as the cmd parameter if you
//		don't want to preserve the original command string. This way this function
//		can operate in-place.
//
// buf_len:
//		A pointer to the size of the buffer we receive in the buf parameter.
//		When you call this function it should contain the size of the buffer.
//		If the function returns with success (C2A_OK) then this size variable
//		is set to the actual size used in the buffer: this is less than or
//		equal to the length of the command string + 1 (because of the
//		terminating zero).
//		If the function doesn't return C2A_OK then the value of this variable
//		isn't modified.
//
// argv:
//		Pointer to an array of uninitialized char pointers.
//		This array should have *argv_len number of items. This function stores
//		the pointers to the individual unescaped/unqouted words of the command
//		into this array along with extra closing NULL item.
// 
// argv_len:
//		A pointer to the length of the array we receive in the argv parameter.
//		When you call the function it should contain the length of argv.
//		If the function returns with success (C2A_OK) then this variable is
//		set to the actual number of items in the argv array not including
//		the closing NULL item. If the function doesn't return C2A_OK then
//		the value of this variable isn't modified.
//
// err_pos:
//		A pointer to an integer that receives the zero-based index of
//		the error location in the cmd string. This parameter can be NULL
//		if you aren't interested in the location of the error.
//		This variable is modified only in case of the C2A_UNMATCHED_QUOT
//		and C2A_LONELY_ESCAPE return values.
C2A_RESULT
cmd2argv(const char* cmd, char* buf, size_t* buf_len,
		char** argv, int* argv_len, int* err_pos);
