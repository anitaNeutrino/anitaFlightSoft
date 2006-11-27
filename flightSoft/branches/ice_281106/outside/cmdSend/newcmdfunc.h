/* newcmdfunc.h - array of command function pointers
 * DO NOT EDIT THIS FILE! It was generated by the newmkcmdfunc.awk
 * MAKE ALL CHANGES TO THE FILE newcmdlist.h
 */

#ifndef _CMDFUNC_H
#define _CMDFUNC_H

struct Cmd {
    char name[32];
    void (*f)(int idx);
};

static void SHUTDOWN_HALT(int idx);
static void REBOOT(int idx);
static void KILL_PROGS(int idx);
static void RESPAWN(int idx);
static void TURN_GPS_ON(int idx);
static void TURN_GPS_OFF(int idx);
static void TURN_RFCM_ON(int idx);
static void TURN_RFCM_OFF(int idx);
static void TURN_CALPULSER_ON(int idx);
static void TURN_CALPULSER_OFF(int idx);
static void TURN_ND_ON(int idx);
static void TURN_ND_OFF(int idx);
static void TURN_ALL_ON(int idx);
static void TURN_ALL_OFF(int idx);
static void TRIG_CALPULSER(int idx);
static void SET_CALPULSER(int idx);
static void SET_SS_GAIN(int idx);
static void GPS(int idx);
static void SIP_GPS(int idx);
static void SUN_SENSORS(int idx);
static void MAGNETOMETER(int idx);
static void ACCEL(int idx);
static void CLEAN_DIRS(int idx);
static void SEND_CONFIG(int idx);
static void DEFAULT_CONFIG(int idx);
static void CHNG_PRIORIT(int idx);
static void SURF(int idx);
static void TURF(int idx);
static void RFCM(int idx);

struct Cmd Cmdarray[] = {
    "NULLCMD               ",	NULL,	/* 0 */
    "NULLCMD               ",	NULL,	/* 1 */
    "NULLCMD               ",	NULL,	/* 2 */
    "NULLCMD               ",	NULL,	/* 3 */
    "NULLCMD               ",	NULL,	/* 4 */
    "NULLCMD               ",	NULL,	/* 5 */
    "NULLCMD               ",	NULL,	/* 6 */
    "NULLCMD               ",	NULL,	/* 7 */
    "NULLCMD               ",	NULL,	/* 8 */
    "NULLCMD               ",	NULL,	/* 9 */
    "NULLCMD               ",	NULL,	/* 10 */
    "NULLCMD               ",	NULL,	/* 11 */
    "NULLCMD               ",	NULL,	/* 12 */
    "NULLCMD               ",	NULL,	/* 13 */
    "NULLCMD               ",	NULL,	/* 14 */
    "NULLCMD               ",	NULL,	/* 15 */
    "NULLCMD               ",	NULL,	/* 16 */
    "NULLCMD               ",	NULL,	/* 17 */
    "NULLCMD               ",	NULL,	/* 18 */
    "NULLCMD               ",	NULL,	/* 19 */
    "NULLCMD               ",	NULL,	/* 20 */
    "NULLCMD               ",	NULL,	/* 21 */
    "NULLCMD               ",	NULL,	/* 22 */
    "NULLCMD               ",	NULL,	/* 23 */
    "NULLCMD               ",	NULL,	/* 24 */
    "NULLCMD               ",	NULL,	/* 25 */
    "NULLCMD               ",	NULL,	/* 26 */
    "NULLCMD               ",	NULL,	/* 27 */
    "NULLCMD               ",	NULL,	/* 28 */
    "NULLCMD               ",	NULL,	/* 29 */
    "NULLCMD               ",	NULL,	/* 30 */
    "NULLCMD               ",	NULL,	/* 31 */
    "NULLCMD               ",	NULL,	/* 32 */
    "NULLCMD               ",	NULL,	/* 33 */
    "NULLCMD               ",	NULL,	/* 34 */
    "NULLCMD               ",	NULL,	/* 35 */
    "NULLCMD               ",	NULL,	/* 36 */
    "NULLCMD               ",	NULL,	/* 37 */
    "NULLCMD               ",	NULL,	/* 38 */
    "NULLCMD               ",	NULL,	/* 39 */
    "NULLCMD               ",	NULL,	/* 40 */
    "NULLCMD               ",	NULL,	/* 41 */
    "NULLCMD               ",	NULL,	/* 42 */
    "NULLCMD               ",	NULL,	/* 43 */
    "NULLCMD               ",	NULL,	/* 44 */
    "NULLCMD               ",	NULL,	/* 45 */
    "NULLCMD               ",	NULL,	/* 46 */
    "NULLCMD               ",	NULL,	/* 47 */
    "NULLCMD               ",	NULL,	/* 48 */
    "NULLCMD               ",	NULL,	/* 49 */
    "NULLCMD               ",	NULL,	/* 50 */
    "NULLCMD               ",	NULL,	/* 51 */
    "NULLCMD               ",	NULL,	/* 52 */
    "NULLCMD               ",	NULL,	/* 53 */
    "NULLCMD               ",	NULL,	/* 54 */
    "NULLCMD               ",	NULL,	/* 55 */
    "NULLCMD               ",	NULL,	/* 56 */
    "NULLCMD               ",	NULL,	/* 57 */
    "NULLCMD               ",	NULL,	/* 58 */
    "NULLCMD               ",	NULL,	/* 59 */
    "NULLCMD               ",	NULL,	/* 60 */
    "NULLCMD               ",	NULL,	/* 61 */
    "NULLCMD               ",	NULL,	/* 62 */
    "NULLCMD               ",	NULL,	/* 63 */
    "NULLCMD               ",	NULL,	/* 64 */
    "NULLCMD               ",	NULL,	/* 65 */
    "NULLCMD               ",	NULL,	/* 66 */
    "NULLCMD               ",	NULL,	/* 67 */
    "NULLCMD               ",	NULL,	/* 68 */
    "NULLCMD               ",	NULL,	/* 69 */
    "NULLCMD               ",	NULL,	/* 70 */
    "NULLCMD               ",	NULL,	/* 71 */
    "NULLCMD               ",	NULL,	/* 72 */
    "NULLCMD               ",	NULL,	/* 73 */
    "NULLCMD               ",	NULL,	/* 74 */
    "NULLCMD               ",	NULL,	/* 75 */
    "NULLCMD               ",	NULL,	/* 76 */
    "NULLCMD               ",	NULL,	/* 77 */
    "NULLCMD               ",	NULL,	/* 78 */
    "NULLCMD               ",	NULL,	/* 79 */
    "NULLCMD               ",	NULL,	/* 80 */
    "NULLCMD               ",	NULL,	/* 81 */
    "NULLCMD               ",	NULL,	/* 82 */
    "NULLCMD               ",	NULL,	/* 83 */
    "NULLCMD               ",	NULL,	/* 84 */
    "NULLCMD               ",	NULL,	/* 85 */
    "NULLCMD               ",	NULL,	/* 86 */
    "NULLCMD               ",	NULL,	/* 87 */
    "NULLCMD               ",	NULL,	/* 88 */
    "NULLCMD               ",	NULL,	/* 89 */
    "NULLCMD               ",	NULL,	/* 90 */
    "NULLCMD               ",	NULL,	/* 91 */
    "NULLCMD               ",	NULL,	/* 92 */
    "NULLCMD               ",	NULL,	/* 93 */
    "NULLCMD               ",	NULL,	/* 94 */
    "NULLCMD               ",	NULL,	/* 95 */
    "NULLCMD               ",	NULL,	/* 96 */
    "NULLCMD               ",	NULL,	/* 97 */
    "NULLCMD               ",	NULL,	/* 98 */
    "NULLCMD               ",	NULL,	/* 99 */
    "NULLCMD               ",	NULL,	/* 100 */
    "NULLCMD               ",	NULL,	/* 101 */
    "NULLCMD               ",	NULL,	/* 102 */
    "NULLCMD               ",	NULL,	/* 103 */
    "NULLCMD               ",	NULL,	/* 104 */
    "NULLCMD               ",	NULL,	/* 105 */
    "NULLCMD               ",	NULL,	/* 106 */
    "NULLCMD               ",	NULL,	/* 107 */
    "NULLCMD               ",	NULL,	/* 108 */
    "NULLCMD               ",	NULL,	/* 109 */
    "NULLCMD               ",	NULL,	/* 110 */
    "NULLCMD               ",	NULL,	/* 111 */
    "NULLCMD               ",	NULL,	/* 112 */
    "NULLCMD               ",	NULL,	/* 113 */
    "NULLCMD               ",	NULL,	/* 114 */
    "NULLCMD               ",	NULL,	/* 115 */
    "NULLCMD               ",	NULL,	/* 116 */
    "NULLCMD               ",	NULL,	/* 117 */
    "NULLCMD               ",	NULL,	/* 118 */
    "NULLCMD               ",	NULL,	/* 119 */
    "NULLCMD               ",	NULL,	/* 120 */
    "NULLCMD               ",	NULL,	/* 121 */
    "NULLCMD               ",	NULL,	/* 122 */
    "NULLCMD               ",	NULL,	/* 123 */
    "NULLCMD               ",	NULL,	/* 124 */
    "NULLCMD               ",	NULL,	/* 125 */
    "NULLCMD               ",	NULL,	/* 126 */
    "NULLCMD               ",	NULL,	/* 127 */
    "NULLCMD               ",	NULL,	/* 128 */
    "SHUTDOWN_HALT         ",	SHUTDOWN_HALT,	/* 129 */
    "REBOOT                ",	REBOOT,	/* 130 */
    "KILL_PROGS            ",	KILL_PROGS,	/* 131 */
    "RESPAWN               ",	RESPAWN,	/* 132 */
    "NULLCMD               ",	NULL,	/* 133 */
    "NULLCMD               ",	NULL,	/* 134 */
    "NULLCMD               ",	NULL,	/* 135 */
    "NULLCMD               ",	NULL,	/* 136 */
    "NULLCMD               ",	NULL,	/* 137 */
    "NULLCMD               ",	NULL,	/* 138 */
    "NULLCMD               ",	NULL,	/* 139 */
    "NULLCMD               ",	NULL,	/* 140 */
    "NULLCMD               ",	NULL,	/* 141 */
    "NULLCMD               ",	NULL,	/* 142 */
    "NULLCMD               ",	NULL,	/* 143 */
    "NULLCMD               ",	NULL,	/* 144 */
    "NULLCMD               ",	NULL,	/* 145 */
    "NULLCMD               ",	NULL,	/* 146 */
    "NULLCMD               ",	NULL,	/* 147 */
    "NULLCMD               ",	NULL,	/* 148 */
    "NULLCMD               ",	NULL,	/* 149 */
    "TURN_GPS_ON           ",	TURN_GPS_ON,	/* 150 */
    "TURN_GPS_OFF          ",	TURN_GPS_OFF,	/* 151 */
    "TURN_RFCM_ON          ",	TURN_RFCM_ON,	/* 152 */
    "TURN_RFCM_OFF         ",	TURN_RFCM_OFF,	/* 153 */
    "TURN_CALPULSER_ON     ",	TURN_CALPULSER_ON,	/* 154 */
    "TURN_CALPULSER_OFF    ",	TURN_CALPULSER_OFF,	/* 155 */
    "TURN_ND_ON            ",	TURN_ND_ON,	/* 156 */
    "TURN_ND_OFF           ",	TURN_ND_OFF,	/* 157 */
    "TURN_ALL_ON           ",	TURN_ALL_ON,	/* 158 */
    "TURN_ALL_OFF          ",	TURN_ALL_OFF,	/* 159 */
    "NULLCMD               ",	NULL,	/* 160 */
    "NULLCMD               ",	NULL,	/* 161 */
    "NULLCMD               ",	NULL,	/* 162 */
    "NULLCMD               ",	NULL,	/* 163 */
    "NULLCMD               ",	NULL,	/* 164 */
    "NULLCMD               ",	NULL,	/* 165 */
    "NULLCMD               ",	NULL,	/* 166 */
    "NULLCMD               ",	NULL,	/* 167 */
    "NULLCMD               ",	NULL,	/* 168 */
    "NULLCMD               ",	NULL,	/* 169 */
    "TRIG_CALPULSER        ",	TRIG_CALPULSER,	/* 170 */
    "SET_CALPULSER         ",	SET_CALPULSER,	/* 171 */
    "SET_SS_GAIN           ",	SET_SS_GAIN,	/* 172 */
    "NULLCMD               ",	NULL,	/* 173 */
    "NULLCMD               ",	NULL,	/* 174 */
    "NULLCMD               ",	NULL,	/* 175 */
    "NULLCMD               ",	NULL,	/* 176 */
    "NULLCMD               ",	NULL,	/* 177 */
    "NULLCMD               ",	NULL,	/* 178 */
    "NULLCMD               ",	NULL,	/* 179 */
    "GPS                   ",	GPS,	/* 180 */
    "SIP_GPS               ",	SIP_GPS,	/* 181 */
    "SUN_SENSORS           ",	SUN_SENSORS,	/* 182 */
    "MAGNETOMETER          ",	MAGNETOMETER,	/* 183 */
    "ACCEL                 ",	ACCEL,	/* 184 */
    "NULLCMD               ",	NULL,	/* 185 */
    "NULLCMD               ",	NULL,	/* 186 */
    "NULLCMD               ",	NULL,	/* 187 */
    "NULLCMD               ",	NULL,	/* 188 */
    "NULLCMD               ",	NULL,	/* 189 */
    "NULLCMD               ",	NULL,	/* 190 */
    "NULLCMD               ",	NULL,	/* 191 */
    "NULLCMD               ",	NULL,	/* 192 */
    "NULLCMD               ",	NULL,	/* 193 */
    "NULLCMD               ",	NULL,	/* 194 */
    "NULLCMD               ",	NULL,	/* 195 */
    "NULLCMD               ",	NULL,	/* 196 */
    "NULLCMD               ",	NULL,	/* 197 */
    "NULLCMD               ",	NULL,	/* 198 */
    "NULLCMD               ",	NULL,	/* 199 */
    "CLEAN_DIRS            ",	CLEAN_DIRS,	/* 200 */
    "NULLCMD               ",	NULL,	/* 201 */
    "NULLCMD               ",	NULL,	/* 202 */
    "NULLCMD               ",	NULL,	/* 203 */
    "NULLCMD               ",	NULL,	/* 204 */
    "NULLCMD               ",	NULL,	/* 205 */
    "NULLCMD               ",	NULL,	/* 206 */
    "NULLCMD               ",	NULL,	/* 207 */
    "NULLCMD               ",	NULL,	/* 208 */
    "NULLCMD               ",	NULL,	/* 209 */
    "SEND_CONFIG           ",	SEND_CONFIG,	/* 210 */
    "DEFAULT_CONFIG        ",	DEFAULT_CONFIG,	/* 211 */
    "NULLCMD               ",	NULL,	/* 212 */
    "NULLCMD               ",	NULL,	/* 213 */
    "NULLCMD               ",	NULL,	/* 214 */
    "NULLCMD               ",	NULL,	/* 215 */
    "NULLCMD               ",	NULL,	/* 216 */
    "NULLCMD               ",	NULL,	/* 217 */
    "NULLCMD               ",	NULL,	/* 218 */
    "NULLCMD               ",	NULL,	/* 219 */
    "CHNG_PRIORIT          ",	CHNG_PRIORIT,	/* 220 */
    "NULLCMD               ",	NULL,	/* 221 */
    "NULLCMD               ",	NULL,	/* 222 */
    "NULLCMD               ",	NULL,	/* 223 */
    "NULLCMD               ",	NULL,	/* 224 */
    "NULLCMD               ",	NULL,	/* 225 */
    "NULLCMD               ",	NULL,	/* 226 */
    "NULLCMD               ",	NULL,	/* 227 */
    "NULLCMD               ",	NULL,	/* 228 */
    "NULLCMD               ",	NULL,	/* 229 */
    "SURF                  ",	SURF,	/* 230 */
    "NULLCMD               ",	NULL,	/* 231 */
    "NULLCMD               ",	NULL,	/* 232 */
    "NULLCMD               ",	NULL,	/* 233 */
    "NULLCMD               ",	NULL,	/* 234 */
    "NULLCMD               ",	NULL,	/* 235 */
    "NULLCMD               ",	NULL,	/* 236 */
    "NULLCMD               ",	NULL,	/* 237 */
    "NULLCMD               ",	NULL,	/* 238 */
    "NULLCMD               ",	NULL,	/* 239 */
    "TURF                  ",	TURF,	/* 240 */
    "NULLCMD               ",	NULL,	/* 241 */
    "NULLCMD               ",	NULL,	/* 242 */
    "NULLCMD               ",	NULL,	/* 243 */
    "NULLCMD               ",	NULL,	/* 244 */
    "NULLCMD               ",	NULL,	/* 245 */
    "NULLCMD               ",	NULL,	/* 246 */
    "NULLCMD               ",	NULL,	/* 247 */
    "NULLCMD               ",	NULL,	/* 248 */
    "NULLCMD               ",	NULL,	/* 249 */
    "RFCM                  ",	RFCM,	/* 250 */
    "NULLCMD               ",	NULL,	/* 251 */
    "NULLCMD               ",	NULL,	/* 252 */
    "NULLCMD               ",	NULL,	/* 253 */
    "NULLCMD               ",	NULL,	/* 254 */
    "NULLCMD               ",	NULL,	/* 255 */
};

int Csize = sizeof(Cmdarray)/sizeof(Cmdarray[0]);

#endif /* _CMDFUNC_H */
