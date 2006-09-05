#/bin/bash
echo $1 >> /tmp/something
echo $2 >> /tmp/somethingElse
#echo $3 >> /tmp/nothingHopefully

headFile=$1
bufferHeadFile=/tmp/buffer/${1}
bodyFile=$2
bufferBodyFile=/tmp/buffer/${2}

#echo "Here $bufferHeadFile $bufferBodyFile"
nice gzip $bufferHeadFile
nice gzip $bufferBodyFile
nice mv ${bufferHeadFile}.gz ${headFile}.gz
nice mv ${bufferBodyFile}.gz ${bodyFile}.gz