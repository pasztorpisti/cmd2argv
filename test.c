#include <stdio.h>
#include <string.h>
#include "cmd2argv.h"

#define ARRAY_LEN(arr)		(sizeof(arr) / sizeof(arr[0]))

#define MAX_CMD_LEN			0x100
#define MAX_ARGV_STR_LEN	0x100
#define MAX_ARGV_LEN		0x80


int test_buf_too_small() {
	static char cmd[] = "echo woof-woof";

	char buf[32];
	size_t buf_len = strlen(cmd)+1;
	char* argv[4];
	int argv_len = 4;

	if (cmd2argv(cmd, buf, &buf_len, argv, &argv_len, NULL) != C2A_OK) {
		printf("%s failed.\n", __FUNCTION__);
		return 0;
	}

	// processing the cmd string would require a buf_len of strlen(cmd)+1
	buf_len = strlen(cmd);
	argv_len = 4;
	C2A_RESULT res = cmd2argv(cmd, buf, &buf_len, argv, &argv_len, NULL);
	if (res != C2A_BUF_TOO_SMALL || buf_len!=strlen(cmd) || argv_len!=4) {
		printf("%s failed.\n", __FUNCTION__);
		return 0;
	}

	return 1;
}

int test_argv_too_small() {
	static char cmd[] = "echo woof-woof";

	char buf[32];
	size_t buf_len = 32;
	char* argv[3];
	int argv_len = 3;

	if (cmd2argv(cmd, buf, &buf_len, argv, &argv_len, NULL) != C2A_OK) {
		printf("%s failed.\n", __FUNCTION__);
		return 0;
	}

	buf_len = 32;
	argv_len = 2;
	C2A_RESULT res = cmd2argv(cmd, buf, &buf_len, argv, &argv_len, NULL);
	if (res != C2A_ARGV_TOO_SMALL || buf_len!=32 || argv_len!=2) {
		printf("%s failed.\n", __FUNCTION__);
		return 0;
	}

	return 1;
}

int test_unmatched_quot() {
	char buf[32];
	size_t buf_len = 32;
	char* argv[4];
	int argv_len = 4;
	int err_pos = -1;

	if (cmd2argv(" ' ", buf, &buf_len, argv, &argv_len, &err_pos) != C2A_UNMATCHED_QUOT ||
			buf_len != 32 || argv_len != 4 || err_pos != 1) {
		printf("%s failed.\n", __FUNCTION__);
		return 0;
	}

	if (cmd2argv("  \" ", buf, &buf_len, argv, &argv_len, &err_pos) != C2A_UNMATCHED_QUOT ||
			buf_len != 32 || argv_len != 4 || err_pos != 2) {
		printf("%s failed.\n", __FUNCTION__);
		return 0;
	}

	return 1;
}

int test_lonely_escape() {
	char buf[32];
	size_t buf_len = 32;
	char* argv[4];
	int argv_len = 4;
	int err_pos = -1;

	if (cmd2argv("   \\", buf, &buf_len, argv, &argv_len, &err_pos) != C2A_LONELY_ESCAPE ||
			buf_len != 32 || argv_len != 4 || err_pos != 3) {
		printf("%s failed.\n", __FUNCTION__);
		return 0;
	}

	return 1;
}


typedef int (*fp_test)();

fp_test builtin_tests[] = {
	test_buf_too_small,
	test_argv_too_small,
	test_unmatched_quot,
	test_lonely_escape
};



void print_argv_as_argv_str(char** argv) {
	int i;
	for (i=0; argv[i]; i++) {
		printf("%s|", argv[i]);
	}
}

// argv_str contains the items of argv. Each argv has a pipe char '|'
// appended. If len(argv)==0 then argv_str is an empty string.
int argv_str_to_argv(char* argv_str, char** argv, size_t argv_len) {
	char* p = argv_str;
	int argc = 0;

	if (argv_len < 1)
		return -1;

	while (*p) {
		if (argv_len < argc + 2)
			return -1;
		argv[argc++] = p;
		while (*p && *p != '|')
			p++;
		if (*p != '|')
			return -1;
		*p++ = 0;
	}

	argv[argc] = NULL;
	return argc;
}

int argc_from_argv(char** argv) {
	int count = 0;
	while (*argv++)
		count++;
	return count;
}

int compare_argv(char** argv1, char** argv2) {
	do {
		if (!*argv1 || !*argv2) {
			if (*argv1 || *argv2)
				return 0;
		} else {
			if (*argv1 && strcmp(*argv1++, *argv2++))
				return 0;
		}
	} while (*argv1);
	return 1;
}

int perform_test(char* cmd, char* argv_str) {
	char* argv[MAX_ARGV_LEN];
	char buf[MAX_CMD_LEN];
	size_t buf_len = sizeof(buf);
	int argv_len = MAX_ARGV_LEN;
	int expected_argc;

	char* test_argv[MAX_ARGV_LEN];

	if (argv_str_to_argv(argv_str, &test_argv[0], MAX_ARGV_LEN) < 0) {
		printf("Error splitting testcase argv_str to argv. Wrong argv_str format?\n");
		return 0;
	}

	C2A_RESULT res = cmd2argv(cmd, buf, &buf_len, &argv[0], &argv_len, NULL);
	if (res == C2A_OK) {
		expected_argc = argc_from_argv(argv);
		if (expected_argc != argv_len) {
			printf("Returned argc(%d) is different from expected argc(%d)\n",
					expected_argc, argv_len);
		}
		else if (compare_argv(argv, test_argv)) {
			return 1;
		}
	} else {
		printf("Error splitting command to args. Return value: %d\n", res);
	}

	printf("cmd:           %s\n", cmd);
	printf("cmd2argv:      ");
	print_argv_as_argv_str(argv);
	printf("\n");
	printf("expected_argv: ");
	print_argv_as_argv_str(test_argv);
	printf("\n");
	return 0;
}

char* remove_newline(char* p) {
	size_t len = strlen(p);
	if (len > 0 && p[len-1] == '\n')
		p[len-1] = 0;
	return p;
}

int main() {
	char cmd[MAX_CMD_LEN];
	char argv_str[MAX_ARGV_STR_LEN];
	FILE* f;
	int passed = 0;
	int failed = 0;
	int lineno = 1;
	size_t i;

	f = fopen("test.txt", "r");
	if (!f) {
		fprintf(stderr, "Error opening test.txt!\n");
		return 1;
	}

	while (fgets(cmd, sizeof(cmd), f) &&
			fgets(argv_str, sizeof(argv_str), f)) {
		if (perform_test(remove_newline(cmd), remove_newline(argv_str))) {
			passed++;
		} else {
			failed++;
			printf("Line number: %d\n\n", lineno);
		}
		lineno += 2;
	}

	fclose(f);

	for (i=0; i<ARRAY_LEN(builtin_tests); i++) {
		if (builtin_tests[i]())
			passed++;
		else
			failed++;
	}

	printf("success: %d\nfailure: %d\n", passed, failed);
	return failed ? 1 : 0;
}
