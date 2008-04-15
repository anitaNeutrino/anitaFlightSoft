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
/* Generated on Sat Jul  1 14:59:22 EDT 2006 */

#include "codelet-dft.h"

#ifdef HAVE_FMA

/* Generated by: ../../../genfft/gen_twiddle_c -fma -reorder-insns -schedule-for-pipeline -simd -compact -variables 4 -pipeline-latency 8 -n 8 -name t1bv_8 -include t1b.h -sign 1 */

/*
 * This function contains 33 FP additions, 24 FP multiplications,
 * (or, 23 additions, 14 multiplications, 10 fused multiply/add),
 * 36 stack variables, and 16 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.9 2006-02-12 23:34:12 athena Exp $
 * $Id: fft.ml,v 1.4 2006-01-05 03:04:27 stevenj Exp $
 * $Id: gen_twiddle_c.ml,v 1.14 2006-02-12 23:34:12 athena Exp $
 */

#include "t1b.h"

static const R *t1bv_8(R *ri, R *ii, const R *W, stride ios, INT m, INT dist)
{
     DVK(KP707106781, +0.707106781186547524400844362104849039284835938);
     INT i;
     R *x;
     x = ii;
     for (i = m; i > 0; i = i - VL, x = x + (VL * dist), W = W + (TWVL * 14), MAKE_VOLATILE_STRIDE(ios)) {
	  V T1, T2, Th, Tj, T5, T7, Ta, Tc;
	  T1 = LD(&(x[0]), dist, &(x[0]));
	  T2 = LD(&(x[WS(ios, 4)]), dist, &(x[0]));
	  Th = LD(&(x[WS(ios, 2)]), dist, &(x[0]));
	  Tj = LD(&(x[WS(ios, 6)]), dist, &(x[0]));
	  T5 = LD(&(x[WS(ios, 1)]), dist, &(x[WS(ios, 1)]));
	  T7 = LD(&(x[WS(ios, 5)]), dist, &(x[WS(ios, 1)]));
	  Ta = LD(&(x[WS(ios, 7)]), dist, &(x[WS(ios, 1)]));
	  Tc = LD(&(x[WS(ios, 3)]), dist, &(x[WS(ios, 1)]));
	  {
	       V T3, Ti, Tk, T6, T8, Tb, Td;
	       T3 = BYTW(&(W[TWVL * 6]), T2);
	       Ti = BYTW(&(W[TWVL * 2]), Th);
	       Tk = BYTW(&(W[TWVL * 10]), Tj);
	       T6 = BYTW(&(W[0]), T5);
	       T8 = BYTW(&(W[TWVL * 8]), T7);
	       Tb = BYTW(&(W[TWVL * 12]), Ta);
	       Td = BYTW(&(W[TWVL * 4]), Tc);
	       {
		    V Tq, T4, Tr, Tl, Tt, T9, Tu, Te, Tw, Ts;
		    Tq = VADD(T1, T3);
		    T4 = VSUB(T1, T3);
		    Tr = VADD(Ti, Tk);
		    Tl = VSUB(Ti, Tk);
		    Tt = VADD(T6, T8);
		    T9 = VSUB(T6, T8);
		    Tu = VADD(Tb, Td);
		    Te = VSUB(Tb, Td);
		    Tw = VADD(Tq, Tr);
		    Ts = VSUB(Tq, Tr);
		    {
			 V Tx, Tv, Tm, Tf;
			 Tx = VADD(Tt, Tu);
			 Tv = VSUB(Tt, Tu);
			 Tm = VSUB(T9, Te);
			 Tf = VADD(T9, Te);
			 {
			      V Tp, Tn, To, Tg;
			      ST(&(x[0]), VADD(Tw, Tx), dist, &(x[0]));
			      ST(&(x[WS(ios, 4)]), VSUB(Tw, Tx), dist, &(x[0]));
			      ST(&(x[WS(ios, 2)]), VFMAI(Tv, Ts), dist, &(x[0]));
			      ST(&(x[WS(ios, 6)]), VFNMSI(Tv, Ts), dist, &(x[0]));
			      Tp = VFMA(LDK(KP707106781), Tm, Tl);
			      Tn = VFNMS(LDK(KP707106781), Tm, Tl);
			      To = VFMA(LDK(KP707106781), Tf, T4);
			      Tg = VFNMS(LDK(KP707106781), Tf, T4);
			      ST(&(x[WS(ios, 1)]), VFMAI(Tp, To), dist, &(x[WS(ios, 1)]));
			      ST(&(x[WS(ios, 7)]), VFNMSI(Tp, To), dist, &(x[WS(ios, 1)]));
			      ST(&(x[WS(ios, 5)]), VFMAI(Tn, Tg), dist, &(x[WS(ios, 1)]));
			      ST(&(x[WS(ios, 3)]), VFNMSI(Tn, Tg), dist, &(x[WS(ios, 1)]));
			 }
		    }
	       }
	  }
     }
     return W;
}

static const tw_instr twinstr[] = {
     VTW(1),
     VTW(2),
     VTW(3),
     VTW(4),
     VTW(5),
     VTW(6),
     VTW(7),
     {TW_NEXT, VL, 0}
};

static const ct_desc desc = { 8, "t1bv_8", twinstr, &GENUS, {23, 14, 10, 0}, 0, 0, 0 };

void X(codelet_t1bv_8) (planner *p) {
     X(kdft_dit_register) (p, t1bv_8, &desc);
}
#else				/* HAVE_FMA */

/* Generated by: ../../../genfft/gen_twiddle_c -simd -compact -variables 4 -pipeline-latency 8 -n 8 -name t1bv_8 -include t1b.h -sign 1 */

/*
 * This function contains 33 FP additions, 16 FP multiplications,
 * (or, 33 additions, 16 multiplications, 0 fused multiply/add),
 * 24 stack variables, and 16 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.9 2006-02-12 23:34:12 athena Exp $
 * $Id: fft.ml,v 1.4 2006-01-05 03:04:27 stevenj Exp $
 * $Id: gen_twiddle_c.ml,v 1.14 2006-02-12 23:34:12 athena Exp $
 */

#include "t1b.h"

static const R *t1bv_8(R *ri, R *ii, const R *W, stride ios, INT m, INT dist)
{
     DVK(KP707106781, +0.707106781186547524400844362104849039284835938);
     INT i;
     R *x;
     x = ii;
     for (i = m; i > 0; i = i - VL, x = x + (VL * dist), W = W + (TWVL * 14), MAKE_VOLATILE_STRIDE(ios)) {
	  V Tl, Tq, Tg, Tr, T5, Tt, Ta, Tu, Ti, Tk, Tj;
	  Ti = LD(&(x[0]), dist, &(x[0]));
	  Tj = LD(&(x[WS(ios, 4)]), dist, &(x[0]));
	  Tk = BYTW(&(W[TWVL * 6]), Tj);
	  Tl = VSUB(Ti, Tk);
	  Tq = VADD(Ti, Tk);
	  {
	       V Td, Tf, Tc, Te;
	       Tc = LD(&(x[WS(ios, 2)]), dist, &(x[0]));
	       Td = BYTW(&(W[TWVL * 2]), Tc);
	       Te = LD(&(x[WS(ios, 6)]), dist, &(x[0]));
	       Tf = BYTW(&(W[TWVL * 10]), Te);
	       Tg = VSUB(Td, Tf);
	       Tr = VADD(Td, Tf);
	  }
	  {
	       V T2, T4, T1, T3;
	       T1 = LD(&(x[WS(ios, 1)]), dist, &(x[WS(ios, 1)]));
	       T2 = BYTW(&(W[0]), T1);
	       T3 = LD(&(x[WS(ios, 5)]), dist, &(x[WS(ios, 1)]));
	       T4 = BYTW(&(W[TWVL * 8]), T3);
	       T5 = VSUB(T2, T4);
	       Tt = VADD(T2, T4);
	  }
	  {
	       V T7, T9, T6, T8;
	       T6 = LD(&(x[WS(ios, 7)]), dist, &(x[WS(ios, 1)]));
	       T7 = BYTW(&(W[TWVL * 12]), T6);
	       T8 = LD(&(x[WS(ios, 3)]), dist, &(x[WS(ios, 1)]));
	       T9 = BYTW(&(W[TWVL * 4]), T8);
	       Ta = VSUB(T7, T9);
	       Tu = VADD(T7, T9);
	  }
	  {
	       V Ts, Tv, Tw, Tx;
	       Ts = VSUB(Tq, Tr);
	       Tv = VBYI(VSUB(Tt, Tu));
	       ST(&(x[WS(ios, 6)]), VSUB(Ts, Tv), dist, &(x[0]));
	       ST(&(x[WS(ios, 2)]), VADD(Ts, Tv), dist, &(x[0]));
	       Tw = VADD(Tq, Tr);
	       Tx = VADD(Tt, Tu);
	       ST(&(x[WS(ios, 4)]), VSUB(Tw, Tx), dist, &(x[0]));
	       ST(&(x[0]), VADD(Tw, Tx), dist, &(x[0]));
	       {
		    V Th, To, Tn, Tp, Tb, Tm;
		    Tb = VMUL(LDK(KP707106781), VSUB(T5, Ta));
		    Th = VBYI(VSUB(Tb, Tg));
		    To = VBYI(VADD(Tg, Tb));
		    Tm = VMUL(LDK(KP707106781), VADD(T5, Ta));
		    Tn = VSUB(Tl, Tm);
		    Tp = VADD(Tl, Tm);
		    ST(&(x[WS(ios, 3)]), VADD(Th, Tn), dist, &(x[WS(ios, 1)]));
		    ST(&(x[WS(ios, 7)]), VSUB(Tp, To), dist, &(x[WS(ios, 1)]));
		    ST(&(x[WS(ios, 5)]), VSUB(Tn, Th), dist, &(x[WS(ios, 1)]));
		    ST(&(x[WS(ios, 1)]), VADD(To, Tp), dist, &(x[WS(ios, 1)]));
	       }
	  }
     }
     return W;
}

static const tw_instr twinstr[] = {
     VTW(1),
     VTW(2),
     VTW(3),
     VTW(4),
     VTW(5),
     VTW(6),
     VTW(7),
     {TW_NEXT, VL, 0}
};

static const ct_desc desc = { 8, "t1bv_8", twinstr, &GENUS, {33, 16, 0, 0}, 0, 0, 0 };

void X(codelet_t1bv_8) (planner *p) {
     X(kdft_dit_register) (p, t1bv_8, &desc);
}
#endif				/* HAVE_FMA */