#!/bin/bash
xterm -T logtail -e "tail -f /var/log/anita.log ; read" &
xterm -T logtail -e "top ; read" &
