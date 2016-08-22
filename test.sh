#!/bin/bash
set -euo pipefail
cd "$( dirname "${BASH_SOURCE[0]}" )"
cc -Wall -o test test.c cmd2argv.c
./test
