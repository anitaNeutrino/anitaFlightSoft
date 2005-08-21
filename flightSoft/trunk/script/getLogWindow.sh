#!/bin/bash
xterm -T logtail -e "tail -f /var/log/anita.log" &
