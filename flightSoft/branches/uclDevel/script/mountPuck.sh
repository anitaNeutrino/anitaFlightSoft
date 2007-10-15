#!/bin/bash

sudo umount /mnt/puck
sudo chmod a-w /mnt/puck
sudo mount -L brick -o defaults,sync /mnt/puck
if df -h | grep -q "puck"
then
	sudo chmod a+wrx /mnt/puck
fi

