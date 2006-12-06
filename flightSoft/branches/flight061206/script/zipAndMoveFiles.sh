#/bin/bash
echo $1 >> /tmp/something
echo $2 >> /tmp/somethingElse
#echo $3 >> /tmp/nothingHopefully

headFile=$1
bufferHeadFile=/tmp/buffer/${1}
bodyFile=$2
bufferBodyFile=/tmp/buffer/${2}

#echo "Here $bufferHeadFile $bufferBodyFile"
bzip2 -c $bufferHeadFile > ${headFile}.bz2
bzip2 -c $bufferBodyFile > ${bodyFile}.bz2
rm $bufferHeadFile
rm $bufferBodyFile