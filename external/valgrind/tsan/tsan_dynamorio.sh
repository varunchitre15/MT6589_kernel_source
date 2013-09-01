#!/bin/bash

DYNAMORIO_ROOT=${DYNAMORIO_ROOT:=$HOME/DynamoRIO}
TS_ROOT=${TS_ROOT:-`dirname $0`}
TS_VARIANT=-debug

TS_FLAGS=" "

for arg in "$@"; do
  case $arg in
    --opt) TS_VARIANT="";;
    --dbg) TS_VARIANT="-debug";;
    --) shift; break;;
    -64) ARCH="amd64"; BITNESS="64";;
    -32) ARCH="x86"; BITNESS="32";;
    -*) TS_FLAGS="$TS_FLAGS $arg";;
    *) break;;
  esac
  shift
done

PROGRAM="$1"
shift
PARAMS="$@"

# detect bitness if not given explicitly.
if [ "$BITNESS" == "" ]; then
  file_format=`objdump -f  $PROGRAM | grep -o 'file format elf.*'`
  echo $file_format
  if [ "$file_format" == "file format elf64-x86-64" ]; then
    BITNESS=64
    ARCH=amd64
  else
    BITNESS=32
    ARCH=x86
  fi
fi

SYMBOLS_FILE="$(mktemp symbols.XXXXXX)"
nm $PROGRAM > $SYMBOLS_FILE
TS_FLAGS="$TS_FLAGS --symbols=$SYMBOLS_FILE"

$DYNAMORIO_ROOT/bin$BITNESS/drdeploy \
   -client $TS_ROOT/bin/$ARCH-linux-debug-ts_dynamorio.so 0 "$TS_FLAGS" \
   $PROGRAM $PARAMS
rm $SYMBOLS_FILE
