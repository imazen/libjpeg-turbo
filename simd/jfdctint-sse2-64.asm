;
; jfdctint.asm - accurate integer FDCT (64-bit SSE2)
;
; Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
; Copyright 2009, 2014, D. R. Commander
;
; Based on
; x86 SIMD extension for IJG JPEG library
; Copyright (C) 1999-2006, MIYASAKA Masaru.
; For conditions of distribution and use, see copyright notice in jsimdext.inc
;
; This file should be assembled with NASM (Netwide Assembler),
; can *not* be assembled with Microsoft's MASM or any compatible
; assembler (including Borland's Turbo Assembler).
; NASM is available from http://nasm.sourceforge.net/ or
; http://sourceforge.net/project/showfiles.php?group_id=6208
;
; This file contains a slow-but-accurate integer implementation of the
; forward DCT (Discrete Cosine Transform). The following code is based
; directly on the IJG's original jfdctint.c; see the jfdctint.c for
; more details.
;
; [TAB8]

%include "jsimdext.inc"
%include "jdct.inc"

; --------------------------------------------------------------------------

%define CONST_BITS      13
%define PASS1_BITS      2

%define DESCALE_P1      (CONST_BITS-PASS1_BITS)
%define DESCALE_P2      (CONST_BITS+PASS1_BITS)

%if CONST_BITS == 13
F_0_298 equ      2446           ; FIX(0.298631336)
F_0_390 equ      3196           ; FIX(0.390180644)
F_0_541 equ      4433           ; FIX(0.541196100)
F_0_765 equ      6270           ; FIX(0.765366865)
F_0_899 equ      7373           ; FIX(0.899976223)
F_1_175 equ      9633           ; FIX(1.175875602)
F_1_501 equ     12299           ; FIX(1.501321110)
F_1_847 equ     15137           ; FIX(1.847759065)
F_1_961 equ     16069           ; FIX(1.961570560)
F_2_053 equ     16819           ; FIX(2.053119869)
F_2_562 equ     20995           ; FIX(2.562915447)
F_3_072 equ     25172           ; FIX(3.072711026)
%else
; NASM cannot do compile-time arithmetic on floating-point constants.
%define DESCALE(x,n)  (((x)+(1<<((n)-1)))>>(n))
F_0_298 equ     DESCALE( 320652955,30-CONST_BITS)       ; FIX(0.298631336)
F_0_390 equ     DESCALE( 418953276,30-CONST_BITS)       ; FIX(0.390180644)
F_0_541 equ     DESCALE( 581104887,30-CONST_BITS)       ; FIX(0.541196100)
F_0_765 equ     DESCALE( 821806413,30-CONST_BITS)       ; FIX(0.765366865)
F_0_899 equ     DESCALE( 966342111,30-CONST_BITS)       ; FIX(0.899976223)
F_1_175 equ     DESCALE(1262586813,30-CONST_BITS)       ; FIX(1.175875602)
F_1_501 equ     DESCALE(1612031267,30-CONST_BITS)       ; FIX(1.501321110)
F_1_847 equ     DESCALE(1984016188,30-CONST_BITS)       ; FIX(1.847759065)
F_1_961 equ     DESCALE(2106220350,30-CONST_BITS)       ; FIX(1.961570560)
F_2_053 equ     DESCALE(2204520673,30-CONST_BITS)       ; FIX(2.053119869)
F_2_562 equ     DESCALE(2751909506,30-CONST_BITS)       ; FIX(2.562915447)
F_3_072 equ     DESCALE(3299298341,30-CONST_BITS)       ; FIX(3.072711026)
%endif

; --------------------------------------------------------------------------
        SECTION SEG_CONST

        alignz  16
        global  EXTN(jconst_fdct_islow_sse2)

EXTN(jconst_fdct_islow_sse2):

PW_F130_F054    times 4 dw  (F_0_541+F_0_765), F_0_541
PW_F054_MF130   times 4 dw  F_0_541, (F_0_541-F_1_847)
PW_MF078_F117   times 4 dw  (F_1_175-F_1_961), F_1_175
PW_F117_F078    times 4 dw  F_1_175, (F_1_175-F_0_390)
PW_MF060_MF089  times 4 dw  (F_0_298-F_0_899),-F_0_899
PW_MF089_F060   times 4 dw -F_0_899, (F_1_501-F_0_899)
PW_MF050_MF256  times 4 dw  (F_2_053-F_2_562),-F_2_562
PW_MF256_F050   times 4 dw -F_2_562, (F_3_072-F_2_562)
PD_DESCALE_P1   times 4 dd  1 << (DESCALE_P1-1)
PD_DESCALE_P2   times 4 dd  1 << (DESCALE_P2-1)
PW_DESCALE_P2X  times 8 dw  1 << (PASS1_BITS-1)

        alignz  16

; --------------------------------------------------------------------------
        SECTION SEG_TEXT
        BITS    64
;
; Perform the forward DCT on one block of samples.
;
; GLOBAL(void)
; jsimd_fdct_islow_sse2 (DCTELEM * data)
;

; r10 = DCTELEM * data

        align   16
        global  EXTN(jsimd_fdct_islow_sse2)

EXTN(jsimd_fdct_islow_sse2):
        push    rbp
        mov     rax,rsp
        mov     rbp,rsp
        collect_args

        ; ---- Pass 1: process rows.

        mov     rdx, r10        ; (DCTELEM *)

        movdqa  xmm8, XMMWORD [XMMBLOCK(0,0,rdx,SIZEOF_DCTELEM)]
        movdqa  xmm9, XMMWORD [XMMBLOCK(1,0,rdx,SIZEOF_DCTELEM)]
        movdqa  xmm2, XMMWORD [XMMBLOCK(2,0,rdx,SIZEOF_DCTELEM)]
        movdqa  xmm11, XMMWORD [XMMBLOCK(3,0,rdx,SIZEOF_DCTELEM)]
        movdqa  xmm4, XMMWORD [XMMBLOCK(4,0,rdx,SIZEOF_DCTELEM)]
        movdqa  xmm13, XMMWORD [XMMBLOCK(5,0,rdx,SIZEOF_DCTELEM)]
        movdqa  xmm5, XMMWORD [XMMBLOCK(6,0,rdx,SIZEOF_DCTELEM)]
        movdqa  xmm15, XMMWORD [XMMBLOCK(7,0,rdx,SIZEOF_DCTELEM)]

        ; xmm8=(00 01 02 03 04 05 06 07), xmm9=(10 11 12 13 14 15 16 17)
        ; xmm2=(20 21 22 23 24 25 26 27), xmm11=(30 31 32 33 34 35 36 37)
        ; xmm4=(40 41 42 43 44 45 46 47), xmm13=(50 51 52 53 54 55 56 57)
        ; xmm5=(60 61 62 63 64 65 66 67), xmm15=(70 71 72 73 74 75 76 77)

        movdqa    xmm12,xmm8            ; transpose coefficients(phase 1)
        punpcklwd xmm8,xmm9             ; xmm8=(00 10 01 11 02 12 03 13)
        punpckhwd xmm12,xmm9            ; xmm12=(04 14 05 15 06 16 07 17)
        movdqa    xmm1,xmm2             ; transpose coefficients(phase 1)
        punpcklwd xmm2,xmm11            ; xmm2=(20 30 21 31 22 32 23 33)
        punpckhwd xmm1,xmm11            ; xmm1=(24 34 25 35 26 36 27 37)

        movdqa    xmm0,xmm4             ; transpose coefficients(phase 1)
        punpcklwd xmm4,xmm13            ; xmm4=(40 50 41 51 42 52 43 53)
        punpckhwd xmm0,xmm13            ; xmm0=(44 54 45 55 46 56 47 57)
        movdqa    xmm3,xmm5             ; transpose coefficients(phase 1)
        punpcklwd xmm5,xmm15            ; xmm5=(60 70 61 71 62 72 63 73)
        punpckhwd xmm3,xmm15            ; xmm3=(64 74 65 75 66 76 67 77)

        movdqa    xmm10,xmm8            ; transpose coefficients(phase 2)
        punpckldq xmm8,xmm2             ; xmm8=(00 10 20 30 01 11 21 31)
        punpckhdq xmm10,xmm2            ; xmm10=(02 12 22 32 03 13 23 33)
        movdqa    xmm14,xmm12           ; transpose coefficients(phase 2)
        punpckldq xmm12,xmm1            ; xmm12=(04 14 24 34 05 15 25 35)
        punpckhdq xmm14,xmm1            ; xmm14=(06 16 26 36 07 17 27 37)

        movdqa    xmm6,xmm4             ; transpose coefficients(phase 2)
        punpckldq xmm4,xmm5             ; xmm4=(40 50 60 70 41 51 61 71)
        punpckhdq xmm6,xmm5             ; xmm6=(42 52 62 72 43 53 63 73)
        movdqa    xmm7,xmm0             ; transpose coefficients(phase 2)
        punpckldq xmm0,xmm3             ; xmm0=(44 54 64 74 45 55 65 75)
        punpckhdq xmm7,xmm3             ; xmm7=(46 56 66 76 47 57 67 77)

        movdqa     xmm9,xmm8            ; transpose coefficients(phase 3)
        punpcklqdq xmm8,xmm4            ; xmm8=(00 10 20 30 40 50 60 70)=data0
        punpckhqdq xmm9,xmm4            ; xmm9=(01 11 21 31 41 51 61 71)=data1
        movdqa     xmm11,xmm10          ; transpose coefficients(phase 3)
        punpcklqdq xmm10,xmm6           ; xmm10=(02 12 22 32 42 52 62 72)=data2
        punpckhqdq xmm11,xmm6           ; xmm11=(03 13 23 33 43 53 63 73)=data3

        movdqa     xmm13,xmm12          ; transpose coefficients(phase 3)
        punpcklqdq xmm12,xmm0           ; xmm12=(04 14 24 34 44 54 64 74)=data4
        punpckhqdq xmm13,xmm0           ; xmm13=(05 15 25 35 45 55 65 75)=data5
        movdqa     xmm15,xmm14          ; transpose coefficients(phase 3)
        punpcklqdq xmm14,xmm7           ; xmm14=(06 16 26 36 46 56 66 76)=data6
        punpckhqdq xmm15,xmm7           ; xmm15=(07 17 27 37 47 57 67 77)=data7

        movdqa  xmm0,xmm8
        paddw   xmm0,xmm15              ; xmm0=data0+data7=tmp0
        movdqa  xmm1,xmm9
        paddw   xmm1,xmm14              ; xmm1=data1+data6=tmp1
        movdqa  xmm2,xmm10
        paddw   xmm2,xmm13              ; xmm2=data2+data5=tmp2
        movdqa  xmm3,xmm11
        paddw   xmm3,xmm12              ; xmm3=data3+data4=tmp3

        psubw   xmm11,xmm12             ; xmm11=data3-data4=tmp4
        psubw   xmm10,xmm13             ; xmm10=data2-data5=tmp5
        psubw   xmm9,xmm14              ; xmm9=data1-data6=tmp6
        psubw   xmm8,xmm15              ; xmm8=data0-data7=tmp7

        ; -- Even part

        movdqa  xmm4,xmm0
        paddw   xmm4,xmm3               ; xmm4=tmp0+tmp3=tmp10
        movdqa  xmm5,xmm1
        paddw   xmm5,xmm2               ; xmm5=tmp1+tmp2=tmp11
        psubw   xmm1,xmm2               ; xmm1=tmp1-tmp2=tmp12
        psubw   xmm0,xmm3               ; xmm0=tmp0-tmp3=tmp13

        movdqa  xmm12,xmm4
        paddw   xmm12,xmm5              ; xmm12=tmp10+tmp11
        psubw   xmm4,xmm5               ; xmm4=tmp10-tmp11

        psllw   xmm12,PASS1_BITS        ; xmm12=out0
        psllw   xmm4,PASS1_BITS         ; xmm4=out4

        ; (Original)
        ; z1 = (tmp12 + tmp13) * 0.541196100;
        ; out2 = z1 + tmp13 * 0.765366865;
        ; out6 = z1 + tmp12 * -1.847759065;
        ;
        ; (This implementation)
        ; out2 = tmp13 * (0.541196100 + 0.765366865) + tmp12 * 0.541196100;
        ; out6 = tmp13 * 0.541196100 + tmp12 * (0.541196100 - 1.847759065);

        movdqa    xmm2,xmm0             ; xmm0=tmp13
        movdqa    xmm6,xmm0
        punpcklwd xmm2,xmm1             ; xmm1=tmp12
        punpckhwd xmm6,xmm1
        movdqa    xmm5,xmm2
        movdqa    xmm0,xmm6
        pmaddwd   xmm2,[rel PW_F130_F054]       ; xmm2=out2L
        pmaddwd   xmm6,[rel PW_F130_F054]       ; xmm6=out2H
        pmaddwd   xmm5,[rel PW_F054_MF130]      ; xmm5=out6L
        pmaddwd   xmm0,[rel PW_F054_MF130]      ; xmm0=out6H

        paddd   xmm2,[rel PD_DESCALE_P1]
        paddd   xmm6,[rel PD_DESCALE_P1]
        psrad   xmm2,DESCALE_P1
        psrad   xmm6,DESCALE_P1
        paddd   xmm5,[rel PD_DESCALE_P1]
        paddd   xmm0,[rel PD_DESCALE_P1]
        psrad   xmm5,DESCALE_P1
        psrad   xmm0,DESCALE_P1

        packssdw  xmm2,xmm6             ; xmm2=out2
        packssdw  xmm5,xmm0             ; xmm5=out6

        ; -- Odd part

        movdqa  xmm6,xmm11              ; xmm11=tmp4
        movdqa  xmm0,xmm10              ; xmm10=tmp5
        paddw   xmm6,xmm9               ; xmm6=tmp4+tmp6=z3
        paddw   xmm0,xmm8               ; xmm0=tmp5+tmp7=z4

        ; (Original)
        ; z5 = (z3 + z4) * 1.175875602;
        ; z3 = z3 * -1.961570560;  z4 = z4 * -0.390180644;
        ; z3 += z5;  z4 += z5;
        ;
        ; (This implementation)
        ; z3 = z3 * (1.175875602 - 1.961570560) + z4 * 1.175875602;
        ; z4 = z3 * 1.175875602 + z4 * (1.175875602 - 0.390180644);

        movdqa    xmm7,xmm6
        movdqa    xmm14,xmm6
        punpcklwd xmm7,xmm0
        punpckhwd xmm14,xmm0
        movdqa    xmm6,xmm7
        movdqa    xmm0,xmm14
        pmaddwd   xmm7,[rel PW_MF078_F117]      ; xmm7=z3L
        pmaddwd   xmm14,[rel PW_MF078_F117]     ; xmm14=z3H
        pmaddwd   xmm6,[rel PW_F117_F078]       ; xmm6=z4L
        pmaddwd   xmm0,[rel PW_F117_F078]       ; xmm0=z4H

        ; (Original)
        ; z1 = tmp4 + tmp7;  z2 = tmp5 + tmp6;
        ; tmp4 = tmp4 * 0.298631336;  tmp5 = tmp5 * 2.053119869;
        ; tmp6 = tmp6 * 3.072711026;  tmp7 = tmp7 * 1.501321110;
        ; z1 = z1 * -0.899976223;  z2 = z2 * -2.562915447;
        ; out7 = tmp4 + z1 + z3;  out5 = tmp5 + z2 + z4;
        ; out3 = tmp6 + z2 + z3;  out1 = tmp7 + z1 + z4;
        ;
        ; (This implementation)
        ; tmp4 = tmp4 * (0.298631336 - 0.899976223) + tmp7 * -0.899976223;
        ; tmp5 = tmp5 * (2.053119869 - 2.562915447) + tmp6 * -2.562915447;
        ; tmp6 = tmp5 * -2.562915447 + tmp6 * (3.072711026 - 2.562915447);
        ; tmp7 = tmp4 * -0.899976223 + tmp7 * (1.501321110 - 0.899976223);
        ; out7 = tmp4 + z3;  out5 = tmp5 + z4;
        ; out3 = tmp6 + z3;  out1 = tmp7 + z4;

        movdqa    xmm15,xmm11           ; xmm11=tmp4
        movdqa    xmm13,xmm11
        punpcklwd xmm15,xmm8            ; xmm8=tmp7
        punpckhwd xmm13,xmm8
        movdqa    xmm8,xmm15
        movdqa    xmm1,xmm13
        pmaddwd   xmm15,[rel PW_MF060_MF089]    ; xmm15=tmp4L
        pmaddwd   xmm13,[rel PW_MF060_MF089]    ; xmm13=tmp4H
        pmaddwd   xmm8,[rel PW_MF089_F060]      ; xmm8=tmp7L
        pmaddwd   xmm1,[rel PW_MF089_F060]      ; xmm1=tmp7H

        paddd   xmm15,xmm7              ; xmm15=out7L
        paddd   xmm13,xmm14             ; xmm13=out7H
        paddd   xmm8,xmm6               ; xmm8=out1L
        paddd   xmm1,xmm0               ; xmm1=out1H

        paddd   xmm15,[rel PD_DESCALE_P1]
        paddd   xmm13,[rel PD_DESCALE_P1]
        psrad   xmm15,DESCALE_P1
        psrad   xmm13,DESCALE_P1
        paddd   xmm8,[rel PD_DESCALE_P1]
        paddd   xmm1,[rel PD_DESCALE_P1]
        psrad   xmm8,DESCALE_P1
        psrad   xmm1,DESCALE_P1

        packssdw  xmm15,xmm13           ; xmm15=out7
        packssdw  xmm8,xmm1             ; xmm8=out1

        movdqa    xmm13,xmm10           ; xmm10=tmp5
        movdqa    xmm1,xmm10
        punpcklwd xmm13,xmm9            ; xmm9=tmp6
        punpckhwd xmm1,xmm9
        movdqa    xmm11,xmm13
        movdqa    xmm3,xmm1
        pmaddwd   xmm13,[rel PW_MF050_MF256]    ; xmm13=tmp5L
        pmaddwd   xmm1,[rel PW_MF050_MF256]     ; xmm1=tmp5H
        pmaddwd   xmm11,[rel PW_MF256_F050]     ; xmm11=tmp6L
        pmaddwd   xmm3,[rel PW_MF256_F050]      ; xmm3=tmp6H

        paddd   xmm13,xmm6              ; xmm13=out5L
        paddd   xmm1,xmm0               ; xmm1=out5H
        paddd   xmm11,xmm7              ; xmm11=out3L
        paddd   xmm3,xmm14              ; xmm3=out3H

        paddd   xmm13,[rel PD_DESCALE_P1]
        paddd   xmm1,[rel PD_DESCALE_P1]
        psrad   xmm13,DESCALE_P1
        psrad   xmm1,DESCALE_P1
        paddd   xmm11,[rel PD_DESCALE_P1]
        paddd   xmm3,[rel PD_DESCALE_P1]
        psrad   xmm11,DESCALE_P1
        psrad   xmm3,DESCALE_P1

        packssdw  xmm13,xmm1            ; xmm13=out5
        packssdw  xmm11,xmm3            ; xmm11=out3

        ; ---- Pass 2: process columns.

        ; Re-order registers so we can reuse the same transpose code
        movdqa    xmm9,xmm8
        movdqa    xmm8,xmm12

        ; xmm8=(00 10 20 30 40 50 60 70), xmm9=(01 11 21 31 41 51 61 71)
        ; xmm2=(02 12 22 32 42 52 62 72), xmm11=(03 13 23 33 43 53 63 73)
        ; xmm4=(04 14 24 34 44 54 64 74), xmm13=(05 15 25 35 45 55 65 75)
        ; xmm5=(06 16 26 36 46 56 66 76), xmm15=(07 17 27 37 47 57 67 77)

        movdqa    xmm12,xmm8            ; transpose coefficients(phase 1)
        punpcklwd xmm8,xmm9             ; xmm8=(00 01 10 11 20 21 30 31)
        punpckhwd xmm12,xmm9            ; xmm12=(40 41 50 51 60 61 70 71)
        movdqa    xmm1,xmm2             ; transpose coefficients(phase 1)
        punpcklwd xmm2,xmm11            ; xmm2=(02 03 12 13 22 23 32 33)
        punpckhwd xmm1,xmm11            ; xmm1=(42 43 52 53 62 63 72 73)

        movdqa    xmm0,xmm4             ; transpose coefficients(phase 1)
        punpcklwd xmm4,xmm13            ; xmm4=(04 05 14 15 24 25 34 35)
        punpckhwd xmm0,xmm13            ; xmm0=(44 45 54 55 64 65 74 75)
        movdqa    xmm3,xmm5             ; transpose coefficients(phase 1)
        punpcklwd xmm5,xmm15            ; xmm5=(06 07 16 17 26 27 36 37)
        punpckhwd xmm3,xmm15            ; xmm3=(46 47 56 57 66 67 76 77)

        movdqa    xmm10,xmm8            ; transpose coefficients(phase 2)
        punpckldq xmm8,xmm2             ; xmm8=(00 01 02 03 10 11 12 13)
        punpckhdq xmm10,xmm2            ; xmm10=(20 21 22 23 30 31 32 33)
        movdqa    xmm14,xmm12           ; transpose coefficients(phase 2)
        punpckldq xmm12,xmm1            ; xmm12=(40 41 42 43 50 51 52 53)
        punpckhdq xmm14,xmm1            ; xmm14=(60 61 62 63 70 71 72 73)

        movdqa    xmm6,xmm4             ; transpose coefficients(phase 2)
        punpckldq xmm4,xmm5             ; xmm4=(04 05 06 07 14 15 16 17)
        punpckhdq xmm6,xmm5             ; xmm6=(24 25 26 27 34 35 36 37)
        movdqa    xmm7,xmm0             ; transpose coefficients(phase 2)
        punpckldq xmm0,xmm3             ; xmm0=(44 45 46 47 54 55 56 57)
        punpckhdq xmm7,xmm3             ; xmm7=(64 65 66 67 74 75 76 77)

        movdqa     xmm9,xmm8            ; transpose coefficients(phase 3)
        punpcklqdq xmm8,xmm4            ; xmm8=(00 01 02 03 04 05 06 07)=data0
        punpckhqdq xmm9,xmm4            ; xmm9=(10 11 12 13 14 15 16 17)=data1
        movdqa     xmm11,xmm10          ; transpose coefficients(phase 3)
        punpcklqdq xmm10,xmm6           ; xmm10=(20 21 22 23 24 25 26 27)=data2
        punpckhqdq xmm11,xmm6           ; xmm11=(30 31 32 33 34 35 36 37)=data3

        movdqa     xmm13,xmm12          ; transpose coefficients(phase 3)
        punpcklqdq xmm12,xmm0           ; xmm12=(40 41 42 43 44 45 46 47)=data4
        punpckhqdq xmm13,xmm0           ; xmm13=(50 51 52 53 54 55 56 57)=data5
        movdqa     xmm15,xmm14          ; transpose coefficients(phase 3)
        punpcklqdq xmm14,xmm7           ; xmm14=(60 61 62 63 64 65 66 67)=data6
        punpckhqdq xmm15,xmm7           ; xmm15=(70 71 72 73 74 75 76 77)=data7

        movdqa  xmm0,xmm8
        paddw   xmm0,xmm15              ; xmm0=data0+data7=tmp0
        movdqa  xmm1,xmm9
        paddw   xmm1,xmm14              ; xmm1=data1+data6=tmp1
        movdqa  xmm2,xmm10
        paddw   xmm2,xmm13              ; xmm2=data2+data5=tmp2
        movdqa  xmm3,xmm11
        paddw   xmm3,xmm12              ; xmm3=data3+data4=tmp3

        psubw   xmm11,xmm12             ; xmm11=data3-data4=tmp4
        psubw   xmm10,xmm13             ; xmm10=data2-data5=tmp5
        psubw   xmm9,xmm14              ; xmm9=data1-data6=tmp6
        psubw   xmm8,xmm15              ; xmm8=data0-data7=tmp7

        ; -- Even part

        movdqa  xmm4,xmm0
        paddw   xmm4,xmm3               ; xmm4=tmp0+tmp3=tmp10
        movdqa  xmm5,xmm1
        paddw   xmm5,xmm2               ; xmm5=tmp1+tmp2=tmp11
        psubw   xmm1,xmm2               ; xmm1=tmp1-tmp2=tmp12
        psubw   xmm0,xmm3               ; xmm0=tmp0-tmp3=tmp13

        movdqa  xmm12,xmm4
        paddw   xmm12,xmm5              ; xmm12=tmp10+tmp11
        psubw   xmm4,xmm5               ; xmm4=tmp10-tmp11

        paddw   xmm12,[rel PW_DESCALE_P2X]
        paddw   xmm4,[rel PW_DESCALE_P2X]
        psraw   xmm12,PASS1_BITS        ; xmm12=out0
        psraw   xmm4,PASS1_BITS         ; xmm4=out4

        ; (Original)
        ; z1 = (tmp12 + tmp13) * 0.541196100;
        ; out2 = z1 + tmp13 * 0.765366865;
        ; out6 = z1 + tmp12 * -1.847759065;
        ;
        ; (This implementation)
        ; out2 = tmp13 * (0.541196100 + 0.765366865) + tmp12 * 0.541196100;
        ; out6 = tmp13 * 0.541196100 + tmp12 * (0.541196100 - 1.847759065);

        movdqa    xmm2,xmm0             ; xmm0=tmp13
        movdqa    xmm6,xmm0
        punpcklwd xmm2,xmm1             ; xmm1=tmp12
        punpckhwd xmm6,xmm1
        movdqa    xmm5,xmm2
        movdqa    xmm0,xmm6
        pmaddwd   xmm2,[rel PW_F130_F054]       ; xmm2=out2L
        pmaddwd   xmm6,[rel PW_F130_F054]       ; xmm6=out2H
        pmaddwd   xmm5,[rel PW_F054_MF130]      ; xmm5=out6L
        pmaddwd   xmm0,[rel PW_F054_MF130]      ; xmm0=out6H

        paddd   xmm2,[rel PD_DESCALE_P2]
        paddd   xmm6,[rel PD_DESCALE_P2]
        psrad   xmm2,DESCALE_P2
        psrad   xmm6,DESCALE_P2
        paddd   xmm5,[rel PD_DESCALE_P2]
        paddd   xmm0,[rel PD_DESCALE_P2]
        psrad   xmm5,DESCALE_P2
        psrad   xmm0,DESCALE_P2

        packssdw  xmm2,xmm6             ; xmm2=out2
        packssdw  xmm5,xmm0             ; xmm5=out6

        ; -- Odd part

        movdqa  xmm6,xmm11              ; xmm11=tmp4
        movdqa  xmm0,xmm10              ; xmm10=tmp5
        paddw   xmm6,xmm9               ; xmm6=tmp4+tmp6=z3
        paddw   xmm0,xmm8               ; xmm0=tmp5+tmp7=z4

        ; (Original)
        ; z5 = (z3 + z4) * 1.175875602;
        ; z3 = z3 * -1.961570560;  z4 = z4 * -0.390180644;
        ; z3 += z5;  z4 += z5;
        ;
        ; (This implementation)
        ; z3 = z3 * (1.175875602 - 1.961570560) + z4 * 1.175875602;
        ; z4 = z3 * 1.175875602 + z4 * (1.175875602 - 0.390180644);

        movdqa    xmm7,xmm6
        movdqa    xmm14,xmm6
        punpcklwd xmm7,xmm0
        punpckhwd xmm14,xmm0
        movdqa    xmm6,xmm7
        movdqa    xmm0,xmm14
        pmaddwd   xmm7,[rel PW_MF078_F117]      ; xmm7=z3L
        pmaddwd   xmm14,[rel PW_MF078_F117]     ; xmm14=z3H
        pmaddwd   xmm6,[rel PW_F117_F078]       ; xmm6=z4L
        pmaddwd   xmm0,[rel PW_F117_F078]       ; xmm0=z4H

        ; (Original)
        ; z1 = tmp4 + tmp7;  z2 = tmp5 + tmp6;
        ; tmp4 = tmp4 * 0.298631336;  tmp5 = tmp5 * 2.053119869;
        ; tmp6 = tmp6 * 3.072711026;  tmp7 = tmp7 * 1.501321110;
        ; z1 = z1 * -0.899976223;  z2 = z2 * -2.562915447;
        ; out7 = tmp4 + z1 + z3;  out5 = tmp5 + z2 + z4;
        ; out3 = tmp6 + z2 + z3;  out1 = tmp7 + z1 + z4;
        ;
        ; (This implementation)
        ; tmp4 = tmp4 * (0.298631336 - 0.899976223) + tmp7 * -0.899976223;
        ; tmp5 = tmp5 * (2.053119869 - 2.562915447) + tmp6 * -2.562915447;
        ; tmp6 = tmp5 * -2.562915447 + tmp6 * (3.072711026 - 2.562915447);
        ; tmp7 = tmp4 * -0.899976223 + tmp7 * (1.501321110 - 0.899976223);
        ; out7 = tmp4 + z3;  out5 = tmp5 + z4;
        ; out3 = tmp6 + z3;  out1 = tmp7 + z4;

        movdqa    xmm15,xmm11           ; xmm11=tmp4
        movdqa    xmm13,xmm11
        punpcklwd xmm15,xmm8            ; xmm8=tmp7
        punpckhwd xmm13,xmm8
        movdqa    xmm8,xmm15
        movdqa    xmm1,xmm13
        pmaddwd   xmm15,[rel PW_MF060_MF089]    ; xmm15=tmp4L
        pmaddwd   xmm13,[rel PW_MF060_MF089]    ; xmm13=tmp4H
        pmaddwd   xmm8,[rel PW_MF089_F060]      ; xmm8=tmp7L
        pmaddwd   xmm1,[rel PW_MF089_F060]      ; xmm1=tmp7H

        paddd   xmm15,xmm7              ; xmm15=out7L
        paddd   xmm13,xmm14             ; xmm13=out7H
        paddd   xmm8,xmm6               ; xmm8=out1L
        paddd   xmm1,xmm0               ; xmm1=out1H

        paddd   xmm15,[rel PD_DESCALE_P2]
        paddd   xmm13,[rel PD_DESCALE_P2]
        psrad   xmm15,DESCALE_P2
        psrad   xmm13,DESCALE_P2
        paddd   xmm8,[rel PD_DESCALE_P2]
        paddd   xmm1,[rel PD_DESCALE_P2]
        psrad   xmm8,DESCALE_P2
        psrad   xmm1,DESCALE_P2

        packssdw  xmm15,xmm13           ; xmm15=out7
        packssdw  xmm8,xmm1             ; xmm8=out1

        movdqa    xmm13,xmm10           ; xmm10=tmp5
        movdqa    xmm1,xmm10
        punpcklwd xmm13,xmm9            ; xmm9=tmp6
        punpckhwd xmm1,xmm9
        movdqa    xmm11,xmm13
        movdqa    xmm3,xmm1
        pmaddwd   xmm13,[rel PW_MF050_MF256]    ; xmm13=tmp5L
        pmaddwd   xmm1,[rel PW_MF050_MF256]     ; xmm1=tmp5H
        pmaddwd   xmm11,[rel PW_MF256_F050]     ; xmm11=tmp6L
        pmaddwd   xmm3,[rel PW_MF256_F050]      ; xmm3=tmp6H

        paddd   xmm13,xmm6               ; xmm13=out5L
        paddd   xmm1,xmm0                ; xmm1=out5H
        paddd   xmm11,xmm7               ; xmm11=out3L
        paddd   xmm3,xmm14               ; xmm3=out3H

        paddd   xmm13,[rel PD_DESCALE_P2]
        paddd   xmm1,[rel PD_DESCALE_P2]
        psrad   xmm13,DESCALE_P2
        psrad   xmm1,DESCALE_P2
        paddd   xmm11,[rel PD_DESCALE_P2]
        paddd   xmm3,[rel PD_DESCALE_P2]
        psrad   xmm11,DESCALE_P2
        psrad   xmm3,DESCALE_P2

        packssdw  xmm13,xmm1            ; xmm13=out5
        packssdw  xmm11,xmm3            ; xmm11=out3

        ; -- Write result

        movdqa  XMMWORD [XMMBLOCK(0,0,rdx,SIZEOF_DCTELEM)], xmm12
        movdqa  XMMWORD [XMMBLOCK(1,0,rdx,SIZEOF_DCTELEM)], xmm8
        movdqa  XMMWORD [XMMBLOCK(2,0,rdx,SIZEOF_DCTELEM)], xmm2
        movdqa  XMMWORD [XMMBLOCK(3,0,rdx,SIZEOF_DCTELEM)], xmm11
        movdqa  XMMWORD [XMMBLOCK(4,0,rdx,SIZEOF_DCTELEM)], xmm4
        movdqa  XMMWORD [XMMBLOCK(5,0,rdx,SIZEOF_DCTELEM)], xmm13
        movdqa  XMMWORD [XMMBLOCK(6,0,rdx,SIZEOF_DCTELEM)], xmm5
        movdqa  XMMWORD [XMMBLOCK(7,0,rdx,SIZEOF_DCTELEM)], xmm15

        uncollect_args
        pop     rbp
        ret

; For some reason, the OS X linker does not honor the request to align the
; segment unless we do this.
        align   16
