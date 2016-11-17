#ifndef SPECTRAL_H
#define SPECTRAL_H

typedef struct
{
	AUD_Int32s fftLen;
	AUD_Float  fs;
	AUD_Int32s *pState[5];
} SpecFeat;

void specfeat_init( SpecFeat **phSpecFeat, AUD_Int32s fftLen, AUD_Float fs );
void specfeat_calc( SpecFeat *pSpecFeat, AUD_Int32s *pFFT, AUD_Int32s *pOut );
void specfeat_free( SpecFeat **phSpecFeat );

#endif // SPECTRAL_H
