/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Huang, Johnathan( Intel Lab )
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#ifndef _FFT_H_
#define _FFT_H_

typedef struct
{
	int    len;
	int    Q;
	int    numbits;
	double *wr;
	double *wi;
	double *xr;
	double *xi;
} Fft_16s;

void fft_init( Fft_16s **phFft, int len, int Q );
void fft_mag( Fft_16s *pFft, short *xr, int inLen, int *Xabs );
void fft_calc( Fft_16s *pFft, short *xr, int inLen, int *Xre, int *Xim );
void fft_free( Fft_16s **phFft );

typedef struct
{
	int    len;
	int    Q;
	double *wr;
	double *wi;
	int    *Xr;
	int    *Xi;
} Ifft_16s;

void ifft_init( Ifft_16s **phIfft, int len, int Q );
void ifft_real( Ifft_16s *pIfft, int *Xr, int *Xi, int inLen, short *xr );
void ifft_free( Ifft_16s **phIfft );

#endif // _FFT_H_
