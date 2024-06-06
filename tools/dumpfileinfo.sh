#!/usr/bin/env bash


# lazily get as much info from a file as possible.
# files are saved in .$FILE-info.d/


if [[ -z "$1" ]]; then
  echo "usage: $0 FILE"
fi

DIRNAME=".$1-info.d"

mkdir -p $DIRNAME

objdump -S $1 > $DIRNAME/objdump.txt
readelf -S $1 > $DIRNAME/readelf.txt
nm $1 > $DIRNAME/nm.txt
