#/bin/bash

file=$1
bufferFile=/tmp/buffer/${1}


nice gzip $bufferFile
nice mv ${bufferFile}.gz ${file}.gz
