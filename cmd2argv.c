#include "cmd2argv.h"

C2A_RESULT
cmd2argv(const char* cmd, char* buf, size_t* buf_len,
		char** argv, int* argv_len, int* err_pos) {
	int argc = 0;
	const char* p = cmd;
	const char* last_quot_pos = NULL;
	char* dest = buf;
	char* dest_end = dest + *buf_len;
	char weak_quoting = 0;

	// argv has to have space at least for the terminating NULL
	if (*argv_len < 1)
		return C2A_ARGV_TOO_SMALL;

	while (*p) {
		while (*p == ' ' || *p == '\t' || *p == '\n')
			p++;
		if (!*p)
			break;

		// It is +2 because the buffer has to be able to hold at least
		// another argv pointer plus a closing NULL argv pointer.
		if (*argv_len < argc + 2)
			return C2A_ARGV_TOO_SMALL;
		argv[argc++] = dest;

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
						*err_pos = (int)(p - cmd);
					return C2A_LONELY_ESCAPE;
				}
				if (dest >= dest_end)
					return C2A_BUF_TOO_SMALL;
				*dest++ = p[1];
				p += 2;
				break;

			case ' ':
			case '\t':
			case '\n':
				if (weak_quoting) {
					if (dest >= dest_end)
						return C2A_BUF_TOO_SMALL;
					*dest++ = *p++;
					break;
				} else {
					p++;
					// double break from both the switch and the while loop
					goto end_string;
				}

			case '\'':
				if (!weak_quoting) {
					// strong quoting
					last_quot_pos = p;
					p++;
					while (*p && *p != '\'' && dest < dest_end)
						*dest++ = *p++;
					if (!*p) {
						if (err_pos)
							*err_pos = (int)(last_quot_pos - cmd);
						return C2A_UNMATCHED_QUOT;
					}
					if (dest >= dest_end)
						return C2A_BUF_TOO_SMALL;
					p++;
					break;
				}
				// no break!!! going for default handling

			default:
				if (dest >= dest_end)
					return C2A_BUF_TOO_SMALL;
				*dest++ = *p++;
				break;
			}
		}
		if (weak_quoting) {
			if (err_pos)
				*err_pos = (int)(last_quot_pos - cmd);
			return C2A_UNMATCHED_QUOT;
		}
end_string:
		if (dest >= dest_end)
			return C2A_BUF_TOO_SMALL;
		*dest++ = 0;
	}

	argv[argc] = NULL;
	*argv_len = argc;
	*buf_len = dest - buf;
	return C2A_OK;
}
