#!/bin/bash
set -euo pipefail

gcc main.c -o rdb

expect s1.exp
expect s2.exp
