
cmd2argv
========

Parses/splits a command string into an `argv` array. Quoting and escaping
are expected to be bash-style. 


Usage
=====

Add the [`cmd2argv.c`](/cmd2argv.c) and [`cmd2argv.h`](/cmd2argv.h) files
to your project. There is a low and a high level interface available.

1. The low level interface (`cmd2argv()`) doesn't perform any allocation.
   You can use it to perform two basic operations:

	1. Calculate the buffer size required to split a given command
	   into an argv array.
	2. Split a specified command into an argv array using the buffers
	   provided by the caller.

2. The high level interface (`cmd2argv_malloc()` and `cmd2argv_free()`)
   uses the low level interface to calculate the required buffer size,
   performs an allocation and then splits the command into the allocated
   buffer. It returns the allocated buffer with the stored results.
   The caller is responsible to free the buffer with `cmd2argv_free()`.


Using the high level interface:
```c
#include "cmd2argv.h"

void example() {
	char** argv;
	if (cmd2argv_malloc("ls -l --color=auto", &argv, NULL, NULL) == C2A_OK) {
		// ... use argv ...
		cmd2argv_free(argv);
	}
}
```

Read the [`cmd2argv.h`](/cmd2argv.h) file for more info about the usage
and the bash-style escaping/quoting supported by this utility.
