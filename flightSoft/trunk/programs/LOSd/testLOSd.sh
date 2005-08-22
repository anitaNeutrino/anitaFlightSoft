#!/bin/bash

xterm -T fakePackets -e "fakePackets ; read" & 
xterm -T LOSd -e "LOSd ; read" & 