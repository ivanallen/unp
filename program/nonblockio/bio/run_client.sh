#!/bin/bash

length=4096
if [ $1 ]; then
	length=$1
fi

time dd if=/dev/zero bs=1024000 count=1 | ./echo -h mars $2 -l $length >/dev/null
