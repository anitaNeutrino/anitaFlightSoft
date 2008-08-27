#!/bin/bash


while [ 1 ];
do
 ./ryanScript.sh
# rm /tmp/tvTest/*
 sleep 10
# for file in /tmp/tvTest??/*; do
#     rm $file
# done
 for file in /tmp/tvTest???/*; do
     rm $file
 done

done
