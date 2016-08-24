#include "cmd2argv.h"

#define ONLY_COUNTING_BUF_LEN (!buf)

C2A_RESULT
cmd2argv(const char* cmd, char* buf, size_t* buf_len,
		char** argv, size_t* argv_len, size_t* err_pos) {
	size_t argc = 0;
	const char* p = cmd;
	const char* last_quot_pos = NULL;
	char* dest = buf;
	char* dest_end = ONLY_COUNTING_BUF_LEN ? NULL : (dest + *buf_len);
	char weak_quoting = 0;

	// argv has to have space at least for the terminating NULL
	if (!ONLY_COUNTING_BUF_LEN && *argv_len < 1)
		return C2A_ARGV_TOO_SMALL;

	while (*p) {
		while (*p == ' ' || *p == '\t' || *p == '\n')
			p++;
		if (!*p)
			break;

		if (!ONLY_COUNTING_BUF_LEN) {
			// It is +2 because the buffer has to be able to hold at least
			// another argv pointer plus a closing NULL argv pointer.
			if (*argv_len < argc + 2)
				return C2A_ARGV_TOO_SMALL;
			argv[argc] = dest;
		}
		argc++;

		while (*p) {
			switch (*p) {
			case '"':
				last_quot_pos = p;
				weak_quoting ^= 1;
				p++;
				break;

			case '\\':
				if (!p[1]) {
					if (err_pos)
						*err_pos = (size_t)(p - cmd);
					return C2A_LONELY_ESCAPE;
				}
				if (!ONLY_COUNTING_BUF_LEN) {
					if (dest >= dest_end)
						return C2A_BUF_TOO_SMALL;
					*dest = p[1];
				}
				dest++;
				p += 2;
				break;

			case ' ':
			case '\t':
			case '\n':
				if (weak_quoting)
					goto default_handling;
				p++;
				// double break from both the switch and the while loop
				goto end_arg;

			case '\'':
				if (weak_quoting)
					goto default_handling;
				// strong quoting
				last_quot_pos = p;
				if (ONLY_COUNTING_BUF_LEN) {
					for (p=p+1; *p && *p != '\''; dest++,p++) {}
				} else {
					for (p=p+1; *p && *p != '\'' && dest < dest_end; dest++,p++)
						*dest = *p;
					if (dest >= dest_end)
						return C2A_BUF_TOO_SMALL;
				}
				if (!*p) {
					if (err_pos)
						*err_pos = (size_t)(last_quot_pos - cmd);
					return C2A_UNMATCHED_QUOT;
				}
				p++;
				break;

default_handling:
			default:
				if (!ONLY_COUNTING_BUF_LEN) {
					if (dest >= dest_end)
						return C2A_BUF_TOO_SMALL;
					*dest = *p;
				}
				dest++;
				p++;
				break;
			}
		}
		if (weak_quoting) {
			if (err_pos)
				*err_pos = (size_t)(last_quot_pos - cmd);
			return C2A_UNMATCHED_QUOT;
		}
end_arg:
		if (!ONLY_COUNTING_BUF_LEN) {
			if (dest >= dest_end)
				return C2A_BUF_TOO_SMALL;
			*dest = 0;
		}
		dest++;
	}

	if (!ONLY_COUNTING_BUF_LEN)
		argv[argc] = NULL;
	if (argv_len)
		*argv_len = argc + 1;
	if (buf_len)
		*buf_len = dest - buf;
	return C2A_OK;
}
