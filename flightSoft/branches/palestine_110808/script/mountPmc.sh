#!/bin/bash

sudo umount /mnt/pmcdata
sudo chmod a-w /mnt/pmcdata
sudo mount -L pmcdata -o defaults /mnt/pmcdata
if df -h | grep -q "pmcdata"
then
	sudo chmod a+wrx /mnt/pmcdata
fi

