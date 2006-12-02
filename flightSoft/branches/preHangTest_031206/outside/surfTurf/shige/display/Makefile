# ------ makefile for base.c
CC = gcc

CFLG = -I./
# LFLG = -L/usr/local/ACQIRIS/usr/lib

radio_disp: radio_disp.h radio_disp.c 
	$(CC) radio_disp.c -o radio_disp -lm `gtk-config --cflags --libs`
