#!/bin/bash
time dd if=/dev/zero bs=1024000 count=1 | ./echo -h mars -l 1024000 $1 >/dev/null
