# newmkcmdfunc.awk - generate the 'newcmdfuncs.h' file from 'newcmdlist.h' 
#
# Marty Olevitch, June '01, editted KJP 7/05

BEGIN {
    for (i=0; i < 256; i++) {
	name[i] = "NULLCMD"
	cmd[i] = "NULL"
    }
}

$1 == "#define" && NF >= 3 {
    if ($2 != "KEYBD_CMD_MAX") {
	name[$3] = $2
	cmd[$3] = $2
    }
}

END {
    printf "/* newcmdfunc.h - array of command function pointers\n"
    printf " * DO NOT EDIT THIS FILE! It was generated by the newmkcmdfunc.awk\n"
    printf " * MAKE ALL CHANGES TO THE FILE newcmdlist.h\n"
    printf " */\n"
    printf "\n"
    printf "#ifndef _CMDFUNC_H\n"
    printf "#define _CMDFUNC_H\n"
    printf "\n"
    printf "struct Cmd {\n"
    printf "    char name[32];\n"
    printf "    void (*f)(int idx);\n"
    printf "};\n"
    printf "\n";

    for (i=0; i<256; i++) {
	if (cmd[i] != "NULL") {
	    printf "static void %s(int idx);\n", cmd[i]
	}
    }
    printf "\n"

    printf "struct Cmd Cmdarray[] = {\n"
    for (i=0; i < 256; i++) {
	printf "    \"%-22s\",\t%s,\t/* %d */\n", name[i], cmd[i], i
    }

    printf "};\n"
    printf "\n"
    printf "int Csize = sizeof(Cmdarray)/sizeof(Cmdarray[0]);\n"
    printf "\n"
    printf "#endif /* _CMDFUNC_H */\n"
}
