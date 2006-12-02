#!/bin/bash
xterm -T logtail -e "tail -f /var/log/anita.log ; read" &
xterm -T top -e "top ; read" &
