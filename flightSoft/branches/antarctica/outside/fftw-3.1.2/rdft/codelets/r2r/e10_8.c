/*
 * Copyright (c) 2003, 2006 Matteo Frigo
 * Copyright (c) 2003, 2006 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* This file was automatically generated --- DO NOT EDIT */
/* Generated on Sun Jul  2 16:46:32 EDT 2006 */

#include "codelet-rdft.h"

#ifdef HAVE_FMA

/* Generated by: ../../../genfft/gen_r2r -fma -reorder-insns -schedule-for-pipeline -compact -variables 4 -pipeline-latency 4 -redft10 -n 8 -name e10_8 -include r2r.h */

/*
 * This function contains 26 FP additions, 18 FP multiplications,
 * (or, 16 additions, 8 multiplications, 10 fused multiply/add),
 * 28 stack variables, and 16 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.9 2006-02-12 23:34:12 athena Exp $
 * $Id: fft.ml,v 1.4 2006-01-05 03:04:27 stevenj Exp $
 * $Id: gen_r2r.ml,v 1.10 2006-02-12 23:34:12 athena Exp $
 */

#include "r2r.h"

static void e10_8(const R *I, R *O, stride is, stride os, INT v, INT ivs, INT ovs)
{
     DK(KP668178637, +0.668178637919298919997757686523080761552472251);
     DK(KP1_662939224, +1.662939224605090474157576755235811513477121624);
     DK(KP198912367, +0.198912367379658006911597622644676228597850501);
     DK(KP1_961570560, +1.961570560806460898252364472268478073947867462);
     DK(KP707106781, +0.707106781186547524400844362104849039284835938);
     DK(KP1_414213562, +1.414213562373095048801688724209698078569671875);
     DK(KP2_000000000, +2.000000000000000000000000000000000000000000000);
     DK(KP414213562, +0.414213562373095048801688724209698078569671875);
     DK(KP1_847759065, +1.847759065022573512256366378793576573644833252);
     INT i;
     for (i = v; i > 0; i = i - 1, I = I + ivs, O = O + ovs, MAKE_VOLATILE_STRIDE(is), MAKE_VOLATILE_STRIDE(os)) {
	  E T3, Te, Tl, Tp, Tm, T6, Tn, T9;
	  {
	       E T4, Tj, Tk, T5, T7, T8;
	       {
		    E T1, T2, Tc, Td;
		    T1 = I[0];
		    T2 = I[WS(is, 7)];
		    Tc = I[WS(is, 4)];
		    Td = I[WS(is, 3)];
		    T4 = I[WS(is, 2)];
		    Tj = T1 + T2;
		    T3 = T1 - T2;
		    Tk = Tc + Td;
		    Te = Tc - Td;
		    T5 = I[WS(is, 5)];
		    T7 = I[WS(is, 1)];
		    T8 = I[WS(is, 6)];
	       }
	       Tl = Tj - Tk;
	       Tp = Tj + Tk;
	       Tm = T4 + T5;
	       T6 = T4 - T5;
	       Tn = T7 + T8;
	       T9 = T7 - T8;
	  }
	  {
	       E Tg, Ti, Tb, Th;
	       {
		    E Tq, To, Ta, Tf;
		    Tq = Tm + Tn;
		    To = Tm - Tn;
		    Ta = T6 + T9;
		    Tf = T6 - T9;
		    O[WS(os, 6)] = KP1_847759065 * (FMA(KP414213562, Tl, To));
		    O[WS(os, 2)] = KP1_847759065 * (FNMS(KP414213562, To, Tl));
		    O[0] = KP2_000000000 * (Tp + Tq);
		    O[WS(os, 4)] = KP1_414213562 * (Tp - Tq);
		    Tg = FNMS(KP707106781, Tf, Te);
		    Ti = FMA(KP707106781, Tf, Te);
		    Tb = FNMS(KP707106781, Ta, T3);
		    Th = FMA(KP707106781, Ta, T3);
	       }
	       O[WS(os, 7)] = KP1_961570560 * (FMA(KP198912367, Th, Ti));
	       O[WS(os, 1)] = KP1_961570560 * (FNMS(KP198912367, Ti, Th));
	       O[WS(os, 5)] = -(KP1_662939224 * (FNMS(KP668178637, Tb, Tg)));
	       O[WS(os, 3)] = KP1_662939224 * (FMA(KP668178637, Tg, Tb));
	  }
     }
}

static const kr2r_desc desc = { 8, "e10_8", {16, 8, 10, 0}, &GENUS, REDFT10, 0, 0, 0, 0 };

void X(codelet_e10_8) (planner *p) {
     X(kr2r_register) (p, e10_8, &desc);
}

#else				/* HAVE_FMA */

/* Generated by: ../../../genfft/gen_r2r -compact -variables 4 -pipeline-latency 4 -redft10 -n 8 -name e10_8 -include r2r.h */

/*
 * This function contains 26 FP additions, 16 FP multiplications,
 * (or, 20 additions, 10 multiplications, 6 fused multiply/add),
 * 28 stack variables, and 16 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.9 2006-02-12 23:34:12 athena Exp $
 * $Id: fft.ml,v 1.4 2006-01-05 03:04:27 stevenj Exp $
 * $Id: gen_r2r.ml,v 1.10 2006-02-12 23:34:12 athena Exp $
 */

#include "r2r.h"

static void e10_8(const R *I, R *O, stride is, stride os, INT v, INT ivs, INT ovs)
{
     DK(KP765366864, +0.765366864730179543456919968060797733522689125);
     DK(KP1_847759065, +1.847759065022573512256366378793576573644833252);
     DK(KP390180644, +0.390180644032256535696569736954044481855383236);
     DK(KP1_961570560, +1.961570560806460898252364472268478073947867462);
     DK(KP2_000000000, +2.000000000000000000000000000000000000000000000);
     DK(KP1_414213562, +1.414213562373095048801688724209698078569671875);
     DK(KP1_111140466, +1.111140466039204449485661627897065748749874382);
     DK(KP1_662939224, +1.662939224605090474157576755235811513477121624);
     DK(KP707106781, +0.707106781186547524400844362104849039284835938);
     INT i;
     for (i = v; i > 0; i = i - 1, I = I + ivs, O = O + ovs, MAKE_VOLATILE_STRIDE(is), MAKE_VOLATILE_STRIDE(os)) {
	  E T3, Tj, Tf, Tk, Ta, Tn, Tc, Tm;
	  {
	       E T1, T2, Td, Te;
	       T1 = I[0];
	       T2 = I[WS(is, 7)];
	       T3 = T1 - T2;
	       Tj = T1 + T2;
	       Td = I[WS(is, 4)];
	       Te = I[WS(is, 3)];
	       Tf = Td - Te;
	       Tk = Td + Te;
	       {
		    E T4, T5, T6, T7, T8, T9;
		    T4 = I[WS(is, 2)];
		    T5 = I[WS(is, 5)];
		    T6 = T4 - T5;
		    T7 = I[WS(is, 1)];
		    T8 = I[WS(is, 6)];
		    T9 = T7 - T8;
		    Ta = KP707106781 * (T6 + T9);
		    Tn = T7 + T8;
		    Tc = KP707106781 * (T6 - T9);
		    Tm = T4 + T5;
	       }
	  }
	  {
	       E Tb, Tg, Tp, Tq;
	       Tb = T3 - Ta;
	       Tg = Tc - Tf;
	       O[WS(os, 3)] = FNMS(KP1_111140466, Tg, KP1_662939224 * Tb);
	       O[WS(os, 5)] = FMA(KP1_662939224, Tg, KP1_111140466 * Tb);
	       Tp = Tj + Tk;
	       Tq = Tm + Tn;
	       O[WS(os, 4)] = KP1_414213562 * (Tp - Tq);
	       O[0] = KP2_000000000 * (Tp + Tq);
	  }
	  {
	       E Th, Ti, Tl, To;
	       Th = T3 + Ta;
	       Ti = Tf + Tc;
	       O[WS(os, 1)] = FNMS(KP390180644, Ti, KP1_961570560 * Th);
	       O[WS(os, 7)] = FMA(KP1_961570560, Ti, KP390180644 * Th);
	       Tl = Tj - Tk;
	       To = Tm - Tn;
	       O[WS(os, 2)] = FNMS(KP765366864, To, KP1_847759065 * Tl);
	       O[WS(os, 6)] = FMA(KP765366864, Tl, KP1_847759065 * To);
	  }
     }
}

static const kr2r_desc desc = { 8, "e10_8", {20, 10, 6, 0}, &GENUS, REDFT10, 0, 0, 0, 0 };

void X(codelet_e10_8) (planner *p) {
     X(kr2r_register) (p, e10_8, &desc);
}

#endif				/* HAVE_FMA */
