#!/bin/bash

echo "disable=0" >/proc/mtrr
echo "disable=1" >/proc/mtrr
echo "base=0x00000000 size=0x80000000 type=write-back" >/proc/mtrr
echo "base=0x80000000 size=0x40000000 type=write-back" >/proc/mtrr
echo "base=0xC0000000 size=0x20000000 type=write-back" >/proc/mtrr
