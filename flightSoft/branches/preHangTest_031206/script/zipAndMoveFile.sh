#/bin/bash

file=$1
bufferFile=/tmp/buffer/${1}


gzip -c $bufferFile > ${file}.gz
rm ${bufferFile}
