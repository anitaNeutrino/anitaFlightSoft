#!/bin/bash

if [ `whoami` == "anita" ]; then
    sudo mii-tool eth1 > /tmp/testneo
else
    mii-tool eth1 > /tmp/testneo
fi

if grep -q "no link" /tmp/testneo; then
    echo "eth1 not present will have to disable neobrick"
    setConfigValue anitaSoft.config global disableNeobrick 1
    chown anita.anita /home/anita/flightSoft/config/*.config
    [ -d /tmp/anita ] || mkdir /tmp/anita
    chown anita.anita /tmp/anita
    echo "######################################################" > /tmp/anita/neobrick.status
    echo "Disabled neobrick because:" >> /tmp/anita/neobrick.status
    cat /tmp/testneo >> /tmp/anita/neobrick.status
    chown anita.anita /tmp/anita/neobrick.status
else
    echo "eth1 is up and maybe Neobrick is alive"
    cd /home/anita/flightSoft/system/neobrick
   sudo -u anita ./patchnb.pl
fi
