#/bin/bash 

if [ -d doc/config ]; then
    echo "doc/config exists"
else
    mkdir doc/config
fi
for file in config/*.config; do
    echo $file
    sed -e '/</s//\&lt;/g' -e '/>/s//\&gt;/g' < $file > doc/$file
done
