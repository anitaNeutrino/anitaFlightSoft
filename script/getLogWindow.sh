#!/bin/bash
xterm -T logtail -e "tail -f /var/log/messages ; read" &
xterm -T top -e "top ; read" &
