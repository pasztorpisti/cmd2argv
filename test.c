/*
This is an ugly piece of code with a lot of repetition but it does the job.
In case of tests I'm not a DRY fan.
*/
#include <stdio.h>
#include <string.h>
#include "cmd2argv.h"

#define LOG_ERROR(fmt, ...)	printf("%s(%d) %s(): " fmt "\n", __FILE__, __LINE__,\
									__FUNCTION__, ## __VA_ARGS__)
#define ARRAY_LEN(arr)		(sizeof(arr) / sizeof(arr[0]))

#define MAX_CMD_LEN			0x100
#define MAX_ARGV_STR_LEN	0x100
#define MAX_ARGV_LEN		0x80


int test_buf_too_small() {
	static char cmd[] = "echo woof-woof";

	char buf[32];
	size_t buf_len = strlen(cmd)+1;
	char* argv[4];
	size_t argv_len = 4;

	if (cmd2argv(cmd, buf, &buf_len, argv, &argv_len, NULL) != C2A_OK) {
		LOG_ERROR("cmd2argv() failed.");
		return 0;
	}

	// processing the cmd string would require a buf_len of strlen(cmd)+1
	buf_len = strlen(cmd);
	argv_len = 4;
	C2A_RESULT res = cmd2argv(cmd, buf, &buf_len, argv, &argv_len, NULL);
	if (res != C2A_BUF_TOO_SMALL || buf_len!=strlen(cmd) || argv_len!=4) {
		LOG_ERROR("Haven't received the expected cmd2argv() error.");
		return 0;
	}

	return 1;
}

int test_argv_too_small() {
	static char cmd[] = "echo woof-woof";

	char buf[32];
	size_t buf_len = 32;
	char* argv[3];
	size_t argv_len = 3;

	if (cmd2argv(cmd, buf, &buf_len, argv, &argv_len, NULL) != C2A_OK) {
		LOG_ERROR("cmd2argv() failed.");
		return 0;
	}

	buf_len = 32;
	argv_len = 2;
	C2A_RESULT res = cmd2argv(cmd, buf, &buf_len, argv, &argv_len, NULL);
	if (res != C2A_ARGV_TOO_SMALL || buf_len!=32 || argv_len!=2) {
		LOG_ERROR("Haven't received the expected cmd2argv() error.");
		return 0;
	}

	return 1;
}

int test_unmatched_quot() {
	char buf[32];
	size_t buf_len = 32;
	char* argv[4];
	size_t argv_len = 4;
	size_t err_pos = 0;

	if (cmd2argv(" ' ", buf, &buf_len, argv, &argv_len, &err_pos) != C2A_UNMATCHED_QUOT ||
			buf_len != 32 || argv_len != 4 || err_pos != 1) {
		LOG_ERROR("Haven't received the expected cmd2argv() error.");
		return 0;
	}

	err_pos = 0;
	if (cmd2argv("  \" ", buf, &buf_len, argv, &argv_len, &err_pos) != C2A_UNMATCHED_QUOT ||
			buf_len != 32 || argv_len != 4 || err_pos != 2) {
		LOG_ERROR("Haven't received the expected cmd2argv() error.");
		return 0;
	}

	// buf_len calculation
	err_pos = 0;
	if (cmd2argv(" ' ", NULL, &buf_len, NULL, &argv_len, &err_pos) != C2A_UNMATCHED_QUOT ||
			buf_len != 32 || argv_len != 4 || err_pos != 1) {
		LOG_ERROR("Haven't received the expected cmd2argv() error.");
		return 0;
	}

	err_pos = 0;
	if (cmd2argv("  \" ", NULL, &buf_len, NULL, &argv_len, &err_pos) != C2A_UNMATCHED_QUOT ||
			buf_len != 32 || argv_len != 4 || err_pos != 2) {
		LOG_ERROR("Haven't received the expected cmd2argv() error.");
		return 0;
	}

	return 1;
}

int test_lonely_escape() {
	char buf[32];
	size_t buf_len = 32;
	char* argv[4];
	size_t argv_len = 4;
	size_t err_pos = 0;

	if (cmd2argv("   \\", buf, &buf_len, argv, &argv_len, &err_pos) != C2A_LONELY_ESCAPE ||
			buf_len != 32 || argv_len != 4 || err_pos != 3) {
		LOG_ERROR("Haven't received the expected cmd2argv() error.");
		return 0;
	}

	// buf_len calculation
	err_pos = 0;
	if (cmd2argv("   \\", NULL, &buf_len, NULL, &argv_len, &err_pos) != C2A_LONELY_ESCAPE ||
			buf_len != 32 || argv_len != 4 || err_pos != 3) {
		LOG_ERROR("Haven't received the expected cmd2argv() error.");
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
size_t argv_str_to_argv(char* argv_str, char** argv, size_t argv_len) {
	char* p = argv_str;
	size_t argc = 0;

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

size_t get_argv_len(char** argv) {
	// starting with count=1 because of the last NULL item in argv
	size_t count = 1;
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

// comparing non-inplace splitting with the testcase argv
int check_splitting_works(char* cmd, char* argv_str) {
	char* argv[MAX_ARGV_LEN];
	char buf[MAX_CMD_LEN];
	size_t buf_len = sizeof(buf);
	size_t argv_len = MAX_ARGV_LEN;
	size_t expected_argv_len;

	char* test_argv[MAX_ARGV_LEN];

	if (argv_str_to_argv(argv_str, &test_argv[0], MAX_ARGV_LEN) < 0) {
		LOG_ERROR("Error splitting testcase argv_str to argv. Wrong argv_str format?");
		return 0;
	}

	C2A_RESULT res = cmd2argv(cmd, buf, &buf_len, &argv[0], &argv_len, NULL);
	if (res == C2A_OK) {
		expected_argv_len = get_argv_len(argv);
		if (expected_argv_len != argv_len) {
			LOG_ERROR("Returned argv_len(%d) is different from expected argv_len(%d)",
					(int)expected_argv_len, (int)argv_len);
		}
		else if (compare_argv(argv, test_argv)) {
			return 1;
		}
	} else {
		LOG_ERROR("Error splitting command to args. Return value: %d", res);
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

// comparing the results of cmd2argv() to that of cmd2argv_malloc()
int check_cmd2argv_malloc(char* cmd) {
	// cmd2argv()
	char* argv[MAX_ARGV_LEN];
	char buf[MAX_CMD_LEN];
	size_t buf_len = sizeof(buf);
	size_t argv_len = MAX_ARGV_LEN;

	// cmd2argv_alloc()
	char** argv2;
	size_t argv_len2;

	C2A_RESULT res = cmd2argv(cmd, buf, &buf_len, &argv[0], &argv_len, NULL);
	if (res != C2A_OK) {
		LOG_ERROR("cmd2argv() failed.");
		return 0;
	}

	res = cmd2argv_malloc(cmd, &argv2, &argv_len2, NULL);
	if (res != C2A_OK) {
		LOG_ERROR("cmd2argv_malloc() failed.");
		return 0;
	}

	if (argv_len != argv_len2) {
		LOG_ERROR("argv_len(%d) != argv_len2(%d)", (int)argv_len, (int)argv_len2);
		cmd2argv_free(argv2);
		return 0;
	}
	if (!compare_argv(argv, argv2)) {
		LOG_ERROR("argv mismatch");
		cmd2argv_free(argv2);
		return 0;
	}
	cmd2argv_free(argv2);
	return 1;
}

// comparing inplace and non-inplace splitting
int check_inplace_splitting(char* cmd) {
	// non-inplace
	char* argv[MAX_ARGV_LEN];
	char buf[MAX_CMD_LEN];
	size_t buf_len = sizeof(buf);
	size_t argv_len = MAX_ARGV_LEN;

	// inplace
	char* argv2[MAX_ARGV_LEN];
	char buf2[MAX_CMD_LEN];
	size_t buf_len2 = sizeof(buf2);
	size_t argv_len2 = MAX_ARGV_LEN;

	// non-inplace
	C2A_RESULT res = cmd2argv(cmd, buf, &buf_len, &argv[0], &argv_len, NULL);
	if (res != C2A_OK) {
		LOG_ERROR("non-inplace cmd2argv() failed.");
		return 0;
	}

	// inplace
	strncpy(buf2, cmd, sizeof(buf2));
	res = cmd2argv(buf2, buf2, &buf_len2, &argv2[0], &argv_len2, NULL);
	if (res != C2A_OK) {
		LOG_ERROR("inplace cmd2argv() failed.");
		return 0;
	}

	if (argv_len != argv_len2) {
		LOG_ERROR("argv_len(%d) != argv_len2(%d)", (int)argv_len, (int)argv_len2);
		return 0;
	}
	if (buf_len != buf_len2) {
		LOG_ERROR("buf_len(%u) != buf_len2(%u)", (unsigned)buf_len, (unsigned)buf_len2);
		return 0;
	}
	if (!compare_argv(argv, argv2)) {
		LOG_ERROR("argv mismatch");
		return 0;
	}
	return 1;
}

// comparing the results of the "buf_len calculation" with
// the output of non-inplace splitting
int check_buf_len_calculation(char* cmd) {
	// non-inplace
	char* argv[MAX_ARGV_LEN];
	char buf[MAX_CMD_LEN];
	size_t buf_len = sizeof(buf);
	size_t argv_len = MAX_ARGV_LEN;

	// buf_len calculation
	size_t buf_len2 = 0;
	size_t argv_len2 = 0;

	// non-inplace
	C2A_RESULT res = cmd2argv(cmd, buf, &buf_len, &argv[0], &argv_len, NULL);
	if (res != C2A_OK) {
		LOG_ERROR("non-inplace cmd2argv() failed.");
		return 0;
	}

	// buf_len calculation
	res = cmd2argv(cmd, NULL, &buf_len2, NULL, &argv_len2, NULL);
	if (res != C2A_OK) {
		LOG_ERROR("cmd2argv() buf_len calculation failed.");
		return 0;
	}

	if (argv_len != argv_len2) {
		LOG_ERROR("argv_len(%d) != argv_len2(%d)", (int)argv_len, (int)argv_len2);
		return 0;
	}
	if (buf_len != buf_len2) {
		LOG_ERROR("buf_len(%u) != buf_len2(%u)", (unsigned)buf_len, (unsigned)buf_len2);
		return 0;
	}
	return 1;
}

int perform_test(char* cmd, char* argv_str) {
	return check_splitting_works(cmd, argv_str) &&
		check_inplace_splitting(cmd) &&
		check_cmd2argv_malloc(cmd) &&
		check_buf_len_calculation(cmd);
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
			printf("test.txt(%d)\n\n", lineno);
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
