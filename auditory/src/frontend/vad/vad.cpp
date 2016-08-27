#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include "AudioDef.h"
#include "AudioUtil.h"
#include "AudioRecog.h"
#include "AudioConfig.h"

#include "math/mmath.h"
#include "type/bufring.h"

typedef struct
{
	AUD_Int16s  *pSpeechBuf;
	AUD_Int16s  *pSpeechFlag;
	AUD_Int32s  speechBufLen;
	AUD_Int32s  speechStartPos;
	AUD_Int32s  speechWritePos;
	AUD_Int32s  speechFrameNum;
	AUD_Int32s  validSpeechSamples;
	AUD_Int32s  tailDepth;

	AUD_Bool    isNeedFlag;
	AUD_Int32s  isSpeechRegion; // 0 - non-speech region, 1 - speech region
	AUD_Int32s  isTransition;

	AUD_Int32s  isTailing;

	AUD_Int32s  smoothWin[SMOOTH_WIN_SIZE];
	AUD_Int32s  smoothWinIdx;

	AUD_Int32s  alertContextWin[ALERT_CONTEXT_SIZE];
	AUD_Int32s  alertWinIdx;

	AUD_BufRing backTrackRing;
	AUD_BufRing backTrackFlag;

#if (VAD_TYPE == AUD_VAD_ENERGY)
	AUD_Double  logEnergyAve;
	AUD_Double  logEnergyDev;
#elif (VAD_TYPE == AUD_VAD_GMM)
	AUD_Double  logEnergyAve;
	void        *hMfccHandle;
	AUD_Feature mfccFeat;
	void        *hSpeechGmm;
	void        *hBackgroundGmm;
	AUD_Double  llrSegment[SEGMENT_WIN_SIZE];
	AUD_Int32s  llrSegmentIdx;
	AUD_Double  energySegment[SEGMENT_WIN_SIZE];
	AUD_Int32s  energySegmentIdx;
#endif
} _AUD_VadState;

AUD_Int32s sig_vadinit( void **phVadHandle, AUD_Int16s *pSpeechBuf, AUD_Int16s *pSpeechFlag, AUD_Int32s speechBufLen, AUD_Int32s backTrackDepth, AUD_Int32s tailDepth )
{
	AUD_ASSERT( phVadHandle && !(*phVadHandle) );

	_AUD_VadState *pVadState = NULL;
	pVadState = (_AUD_VadState*)calloc( sizeof(_AUD_VadState), 1 );
	AUD_ASSERT( pVadState );

	AUD_Int32s ret;

	if ( backTrackDepth < 0 )
	{
		backTrackDepth = BACKTRACK_SIZE;
	}
	backTrackDepth = backTrackDepth + 5; // add 5 frame for possible consonant

	if ( tailDepth < 0 )
	{
		tailDepth = SPEECH_TO_NONSPEECH_SMOOTH;
	}

	pVadState->backTrackRing.ringSize = backTrackDepth;
	pVadState->backTrackRing.bufSize  = VAD_FRAME_LEN;
	ret = createBufRing( &(pVadState->backTrackRing) );
	AUD_ASSERT( ret == 0 );

	pVadState->isNeedFlag = AUD_FALSE;
	if ( pSpeechFlag )
	{
		pVadState->isNeedFlag = AUD_TRUE;
		pVadState->backTrackFlag.ringSize = backTrackDepth;
		pVadState->backTrackFlag.bufSize  = 1;
		ret = createBufRing( &(pVadState->backTrackFlag) );
		AUD_ASSERT( ret == 0 );
	}

	pVadState->pSpeechBuf         = pSpeechBuf;
	pVadState->pSpeechFlag        = pSpeechFlag;
	pVadState->speechBufLen       = speechBufLen;
	pVadState->tailDepth          = tailDepth;
	pVadState->speechStartPos     = pVadState->backTrackRing.ringSize;
	pVadState->speechWritePos     = pVadState->speechStartPos;
	pVadState->speechFrameNum     = pVadState->speechBufLen / VAD_FRAME_LEN / BYTES_PER_SAMPLE;
	pVadState->validSpeechSamples = 0;

	pVadState->smoothWinIdx       = 0;
	pVadState->isSpeechRegion     = 0;

#if (VAD_TYPE == AUD_VAD_ENERGY)

	pVadState->logEnergyAve    = VAD_ENERGY_INITIAL;
	pVadState->logEnergyDev	   = 0.0;

#elif (VAD_TYPE == AUD_VAD_GMM)

	pVadState->logEnergyAve    = VAD_ENERGY_INITIAL;

	AUD_Error error = AUD_ERROR_NONE;
	error = mfcc16s32s_init( &(pVadState->hMfccHandle), FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	pVadState->mfccFeat.featureMatrix.rows     = 1;
	pVadState->mfccFeat.featureMatrix.cols     = MFCC_FEATDIM;
	pVadState->mfccFeat.featureMatrix.dataType = AUD_DATATYPE_INT32S;
	ret = createMatrix( &(pVadState->mfccFeat.featureMatrix) );
	AUD_ASSERT( ret == 0 );

	pVadState->mfccFeat.featureNorm.len      = 1;
	pVadState->mfccFeat.featureNorm.dataType = AUD_DATATYPE_INT64S;
	ret = createVector( &(pVadState->mfccFeat.featureNorm) );
	AUD_ASSERT( ret == 0 );

	// load speech GMM model
	// AUDLOG( "import speech gmm model from: [%s]\n", VAD_SPEECH_MODEL_FILE );

	FILE *fpSpeechGmm = fopen( (char*)VAD_SPEECH_MODEL_FILE, "rb" );
	if ( fpSpeechGmm == NULL )
	{
		AUDLOG( "cannot open gmm model file: [%s]\n", VAD_SPEECH_MODEL_FILE );
		return AUD_ERROR_IOFAILED;
	}
	error = gmm_import( &(pVadState->hSpeechGmm), fpSpeechGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	fclose( fpSpeechGmm );
	fpSpeechGmm = NULL;

	// load background GMM model
	// AUDLOG( "import background gmm model from: [%s]\n", VAD_BACKGROUND_MODEL_FILE );
	FILE *fpBackgroundGmm = fopen( (char*)VAD_BACKGROUND_MODEL_FILE, "rb" );
	if ( fpBackgroundGmm == NULL )
	{
		AUDLOG( "cannot open gmm model file: [%s]\n", VAD_BACKGROUND_MODEL_FILE );
		return AUD_ERROR_IOFAILED;
	}
	error = gmm_import( &(pVadState->hBackgroundGmm), fpBackgroundGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	fclose( fpBackgroundGmm );
	fpBackgroundGmm = NULL;

#endif

	*phVadHandle = pVadState;

	return 0;
}

AUD_Int32s sig_vaddeinit( void **phVadHandle )
{
	AUD_ASSERT( phVadHandle && *phVadHandle );

	_AUD_VadState *pVadState = (_AUD_VadState*)(*phVadHandle);
	AUD_Int32s    ret        = 0;

	ret = destroyBufRing( &(pVadState->backTrackRing) );
	AUD_ASSERT( ret == 0 );

	if ( pVadState->isNeedFlag )
	{
		ret = destroyBufRing( &(pVadState->backTrackFlag) );
		AUD_ASSERT( ret == 0 );
	}

#if (VAD_TYPE == AUD_VAD_GMM)

	AUD_Error error = AUD_ERROR_NONE;
	error = mfcc16s32s_deinit( &(pVadState->hMfccHandle) );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	ret = destroyMatrix( &(pVadState->mfccFeat.featureMatrix) );
	AUD_ASSERT( ret == 0 );

	ret = destroyVector( &(pVadState->mfccFeat.featureNorm) );
	AUD_ASSERT( ret == 0 );

	error = gmm_free( &(pVadState->hSpeechGmm) );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	error = gmm_free( &(pVadState->hBackgroundGmm) );
	AUD_ASSERT( error == AUD_ERROR_NONE );

#endif

	free( pVadState );
	pVadState = NULL;

	*phVadHandle = NULL;

	return 0;
}

/* if speech return 1, need more data return -1, else return 0 */
AUD_Int32s sig_framecheck( void *hVadHandle, AUD_Int16s *pFrameBuf, AUD_Int16s *pFlag )
{
	AUD_ASSERT( hVadHandle && pFrameBuf && pFlag );

	_AUD_VadState *pVadState = (_AUD_VadState*)hVadHandle;

#if (VAD_TYPE == AUD_VAD_ENERGY)

	AUD_Int32s    i;
	AUD_Double    x;
	AUD_Double    s, logEnergyStd;

	AUD_Int16s *pPreBuf = NULL;
	pPreBuf = (AUD_Int16s*)calloc( FRAME_LEN * BYTES_PER_SAMPLE, 1 );
	AUD_ASSERT( pPreBuf );

	sig_preemphasis( pFrameBuf, pPreBuf, FRAME_LEN );
	x = 0.;
	// AUDLOG( "frame: \n" );
	for ( i = 0; i < FRAME_LEN; i++ )
	{
		x += pow( pPreBuf[i] / 32768., 2. );
		// AUDLOG( "%.2f, ", pFrameBuf[i] / 32768. );
	}
	// AUDLOG( "\n" );

	free( pPreBuf );
	pPreBuf = NULL;

	s = 16. * log( 1. + x / 64. ) / log( 2. );

	logEnergyStd = sqrt( pVadState->logEnergyDev );
	// NOTE: here make assumption that the std variance of sound is log-dependent on sound mean energy, it's an empirical result. Pls challege us if you don't think so.
	if ( logEnergyStd < log10( pVadState->logEnergyAve  * 10000. ) / 900. )
	{
		logEnergyStd = log10( pVadState->logEnergyAve  * 10000. ) / 900.;
	}

	// AUDLOG( "x: %.6f, s: %.5f, log_energy_ave: %.5f, log_energy_std: %.5f\n", x, s, pVadState->logEnergyAve, logEngergyStd );
	if ( s > pVadState->logEnergyAve + logEnergyStd * 3. )
	{
		return 1;
	}
	else
	{
		// update mean square deviation
		if ( ( s - pVadState->logEnergyAve ) * ( s - pVadState->logEnergyAve ) < pVadState->logEnergyDev )
		{
			pVadState->logEnergyDev  = pVadState->logEnergyDev  + ( 1. - LAMBDA_L ) * ( ( s - pVadState->logEnergyAve ) * ( s - pVadState->logEnergyAve ) - pVadState->logEnergyDev );
		}
		else
		{
			pVadState->logEnergyDev  = pVadState->logEnergyDev  + ( 1. - LAMBDA_H ) * ( ( s - pVadState->logEnergyAve ) * ( s - pVadState->logEnergyAve ) - pVadState->logEnergyDev );
		}
		// AUDLOG( "speech frame logEnergyDev: %.4f, \n", pVadState->logEnergyDev );
		// update long term average here
		if ( s < pVadState->logEnergyAve )
		{
			pVadState->logEnergyAve = pVadState->logEnergyAve + ( 1. - LAMBDA_L ) * ( s - pVadState->logEnergyAve );
		}
		else
		{
			pVadState->logEnergyAve = pVadState->logEnergyAve + ( 1. - LAMBDA_H ) * ( s - pVadState->logEnergyAve );
		}

		return 0;
	}

#elif (VAD_TYPE == AUD_VAD_GMM)

	AUD_Error  error           = AUD_ERROR_NONE;
	AUD_Double speechScore     = 0.;
	AUD_Double backgroundScore = 0.;
	AUD_Double score           = 0.;
	AUD_Double sumScore        = 0.;

	AUD_Int16s *pPreBuf = NULL;
	pPreBuf = (AUD_Int16s*)calloc( FRAME_LEN * BYTES_PER_SAMPLE, 1 );
	AUD_ASSERT( pPreBuf );

	sig_preemphasis( pFrameBuf, pPreBuf, FRAME_LEN );

	// Energy constraint
	AUD_Int32s    i;
	AUD_Double    energy    = 0.;
	AUD_Double    logEnergy = 0.;

	for ( i = 0; i < FRAME_LEN; i++ )
	{
		energy += pow( pPreBuf[i] / 32768., 2 );
	}
	logEnergy = 16. * log( 1 + energy / 64. ) / log( 2 );
	// AUDLOG( "\n Frame Energy: %.4f \n", energy );

	error = mfcc16s32s_calc( pVadState->hMfccHandle, pPreBuf, FRAME_LEN, &(pVadState->mfccFeat) );
	if ( error == AUD_ERROR_MOREDATA )
	{
		free( pPreBuf );
		pPreBuf = NULL;
		return -1;
	}
	AUD_ASSERT( error == AUD_ERROR_NONE );

	free( pPreBuf );
	pPreBuf = NULL;

	speechScore = gmm_llr( pVadState->hSpeechGmm, &(pVadState->mfccFeat.featureMatrix), 0, NULL );

	backgroundScore = gmm_llr( pVadState->hBackgroundGmm, &(pVadState->mfccFeat.featureMatrix), 0, NULL );

	score = speechScore - backgroundScore;
	if ( score > -10.0 )
	{
		pVadState->llrSegment[pVadState->llrSegmentIdx] = score + 10.;
	}
	else
	{
		pVadState->llrSegment[pVadState->llrSegmentIdx] = 0.;
	}
	pVadState->llrSegmentIdx = ( pVadState->llrSegmentIdx + 1 ) % SEGMENT_WIN_SIZE;

	pVadState->energySegment[pVadState->energySegmentIdx] = logEnergy;
	pVadState->energySegmentIdx = ( pVadState->energySegmentIdx + 1 ) % SEGMENT_WIN_SIZE;

	for ( i = 0; i < SEGMENT_WIN_SIZE; i++)
	{
		sumScore += pVadState->llrSegment[i] * ( pVadState->energySegment[i] / pVadState->logEnergyAve );
	}

	// FILE *fp = fopen( "./frame_score.log", "ab+" );
	// fprintf( fp, "%.4f, %.4f \n", score, sumScore );
	// fclose( fp );
	// fp = NULL;

	if ( sumScore > VAD_THRSHOLD )
	{
#if (VAD_FLAG_TYPE == 1)
		*pFlag = 1;
#elif (VAD_FLAG_TYPE == 0)
		*pFlag = ( score > 0. ? 1 : 0 );
#endif

		return 1;
	}
	else
	{
#if (VAD_FLAG_TYPE == 1)
		*pFlag = 0;
#elif (VAD_FLAG_TYPE == 0)
		*pFlag = ( score > 0. ? 1 : 0 );
#endif

		if ( logEnergy < 3. * pVadState->logEnergyAve )
		{
			if ( logEnergy < pVadState->logEnergyAve )
			{
				pVadState->logEnergyAve = pVadState->logEnergyAve + ( 1 - LAMBDA_L ) * ( logEnergy - pVadState->logEnergyAve );
			}
			else
			{
				pVadState->logEnergyAve = pVadState->logEnergyAve + ( 1 - LAMBDA_H ) * ( logEnergy - pVadState->logEnergyAve );
			}

			pVadState->logEnergyAve = AUD_MAX( pVadState->logEnergyAve, 0.0001 );
			// AUDLOG( "noise average energy updated to: %f\n", pVadState->logEnergyAve );
		}
		return 0;
	}
#endif
}


/* if speech ready return sample number of speech, else return 0 */
AUD_Int32s sig_vadupdate( void *hVadHandle, AUD_Int16s *pFrameBuf, AUD_Int32s *pStart )
{
	AUD_ASSERT( hVadHandle );
	AUD_ASSERT( pFrameBuf );
	AUD_ASSERT( pStart );

	_AUD_VadState *pVadState = (_AUD_VadState*)hVadHandle;

	AUD_ASSERT( pVadState->pSpeechBuf != NULL && pVadState->speechBufLen > 0 );

	AUD_Int32s    ret;
	AUD_Int32s    i, j;
	AUD_Int16s    flag = 0;

	ret = sig_framecheck( hVadHandle, pFrameBuf, &flag );
	if ( ret == -1 )
	{
		*pStart = -1;
		return 0;
	}

	if ( !pVadState->isSpeechRegion ) // in non-speech region
	{
		if ( !pVadState->isTailing )
		{
			if ( ret == 1 )
			{
				AUD_Int32s contextCnt = 0;
				for ( i = 0; i < ALERT_CONTEXT_SIZE; i++ )
				{
					contextCnt += pVadState->alertContextWin[i];
				}

				if ( contextCnt > ALERT_CONTEXT_THRESHOLD )
				{
					// copy to speech buffer
					memmove( pVadState->pSpeechBuf + pVadState->speechWritePos * VAD_FRAME_LEN, pFrameBuf, VAD_FRAME_LEN * BYTES_PER_SAMPLE );
					if ( pVadState->isNeedFlag )
					{
						pVadState->pSpeechFlag[pVadState->speechWritePos] = flag;
					}
					(pVadState->speechWritePos)++;

					pVadState->validSpeechSamples += VAD_FRAME_LEN;
					// AUDLOG( "Add SpeechBuf, validSpeechSamples: %d\n", pVadState->validSpeechSamples );

					if ( pVadState->isTransition == 0 )
					{
						pVadState->isTransition = 1;
					}
				}
				else
				{
					// one negative vote for fore-context
					pVadState->alertContextWin[pVadState->alertWinIdx] = 0;
					pVadState->alertWinIdx                             = ( pVadState->alertWinIdx + 1 ) % ALERT_CONTEXT_SIZE;

					// AUDLOG( "insert to back track ring" );
					writeToBufRing( &(pVadState->backTrackRing), pFrameBuf );
					if ( pVadState->isNeedFlag )
					{
						writeToBufRing( &(pVadState->backTrackFlag), &flag );
					}
				}
			}
			else
			{
				// one support vote for fore-context
				pVadState->alertContextWin[pVadState->alertWinIdx] = 1;
				pVadState->alertWinIdx                             = ( pVadState->alertWinIdx + 1 ) % ALERT_CONTEXT_SIZE;

				// AUDLOG( "insert to back track ring" );
				writeToBufRing( &(pVadState->backTrackRing), pFrameBuf );
				if ( pVadState->isNeedFlag )
				{
					writeToBufRing( &(pVadState->backTrackFlag), &flag );
				}
			}

			if ( pVadState->isTransition == 1 )
			{
				if ( ret == 1 )
				{
					pVadState->smoothWin[pVadState->smoothWinIdx] = 1;
				}
				else
				{
					pVadState->smoothWin[pVadState->smoothWinIdx] = 0;
				}
				pVadState->smoothWinIdx++;

				if ( pVadState->smoothWinIdx == NONSPEECH_TO_SPEECH_SMOOTH )
				{
					AUD_Int32s speechCnt = 0;
					for ( i = 0; i < NONSPEECH_TO_SPEECH_SMOOTH; i++ )
					{
						speechCnt += pVadState->smoothWin[i];
					}

					if ( speechCnt >= NONSPEECH_TO_SPEECH_THRESHOLD )
					{
						// AUDLOG( "smooth result: speech start\n" );
						pVadState->isSpeechRegion = 1;
						pVadState->smoothWinIdx   = 0;
						for ( i = 0; i < SMOOTH_WIN_SIZE; i++ )
						{
							pVadState->smoothWin[i] = 0;
						}

						pVadState->alertWinIdx  = 0;
						for ( i = 0; i < ALERT_CONTEXT_SIZE; i++ )
						{
							pVadState->alertContextWin[i] = 0;
						}

						pVadState->isTransition = 0;
					}
					else
					{
						// AUDLOG( "smooth result: non-speech\n" );
						pVadState->smoothWinIdx = 0;
						for ( i = 0; i < SMOOTH_WIN_SIZE; i++ )
						{
							pVadState->smoothWin[i] = 0;
						}

						for ( i = pVadState->speechStartPos; i < pVadState->speechWritePos; i++)
						{
							// AUDLOG( "insert to buf ring\n" );
							writeToBufRing( &(pVadState->backTrackRing), pVadState->pSpeechBuf + i * VAD_FRAME_LEN );
							if ( pVadState->isNeedFlag )
							{
								writeToBufRing( &(pVadState->backTrackFlag), pVadState->pSpeechFlag + i );
							}
						}

						pVadState->speechWritePos     = pVadState->speechStartPos;
						pVadState->validSpeechSamples = 0;

						pVadState->isTransition       = 0;
					}
				}
			}
		}
		else // trailing
		{
			if ( ret == 0 )
			{
				pVadState->alertContextWin[pVadState->alertWinIdx] = 1;
			}
			else
			{
				pVadState->alertContextWin[pVadState->alertWinIdx] = 0;
			}

			pVadState->alertWinIdx++;

			if ( pVadState->alertWinIdx == ALERT_CONTEXT_SIZE )
			{
				AUD_Int32s contextCnt = 0;
				for ( i = 0; i < ALERT_CONTEXT_SIZE; i++ )
				{
					contextCnt += pVadState->alertContextWin[i];
				}
				if ( contextCnt > ALERT_CONTEXT_THRESHOLD )
				{
					// AUDLOG( "end of trailing\n" );
					// suspect speech segment, return to user
					// moving leading noisy buffer to speech buffer head
					AUD_Int32s spBackTracking = pVadState->backTrackRing.ringSize;
					AUD_Int32s validSampleNum = 0;

					if ( getFilledBufNum( &(pVadState->backTrackRing) ) < spBackTracking )
					{
						spBackTracking = getFilledBufNum( &(pVadState->backTrackRing) );
					}
					i = pVadState->speechStartPos - getFilledBufNum( &(pVadState->backTrackRing) );

					// AUDLOG( "\n%d\n", getFilledBufNum( &(pVadState->backTrackRing) ) );
					for ( j = i; j < pVadState->speechStartPos; j++ )
					{
						ret = readFromBufRing( &(pVadState->backTrackRing), pVadState->pSpeechBuf + j * VAD_FRAME_LEN );
						if ( pVadState->isNeedFlag )
						{
							ret = readFromBufRing( &(pVadState->backTrackFlag), pVadState->pSpeechFlag + j );
						}
					}

					*pStart        = ( pVadState->speechStartPos - 5 ) * VAD_FRAME_LEN;
					validSampleNum = pVadState->validSpeechSamples - ( SPEECH_TO_NONSPEECH_SMOOTH - pVadState->tailDepth ) * VAD_FRAME_LEN;

					pVadState->isTailing      = 0;
					pVadState->isSpeechRegion = 0;
					pVadState->smoothWinIdx   = 0;
					for ( i = 0; i < SMOOTH_WIN_SIZE; i++ )
					{
						pVadState->smoothWin[i] = 0;
					}

					pVadState->alertWinIdx = 0;
					for ( i = 0; i < ALERT_CONTEXT_SIZE; i++ )
					{
						pVadState->alertContextWin[i] = 0;
					}

					pVadState->speechWritePos     = pVadState->speechStartPos;
					pVadState->validSpeechSamples = 0;

					// AUDLOG( "clear back track ring\n" );
					clearBufRing( &(pVadState->backTrackRing) );
					if ( pVadState->isNeedFlag )
					{
						clearBufRing( &(pVadState->backTrackFlag) );
					}

					return validSampleNum;
				}
				else
				{
					// suspect not in alerting context
					// AUDLOG( "suspect not in alerting context: %d", ALERT_CONTEXT_THRESHOLD );
					pVadState->isTailing      = 0;
					pVadState->isSpeechRegion = 0;
					pVadState->smoothWinIdx   = 0;
					pVadState->isTransition   = 0;
					for ( i = 0; i < SMOOTH_WIN_SIZE; i++ )
					{
						pVadState->smoothWin[i] = 0;
					}

					pVadState->alertWinIdx = 0;
					for ( i = 0; i < ALERT_CONTEXT_SIZE; i++ )
					{
						pVadState->alertContextWin[i] = 0;
					}

					pVadState->speechWritePos = pVadState->speechStartPos;
					pVadState->validSpeechSamples = 0;

					// AUDLOG( "clear back track ring\n" );
					clearBufRing( &(pVadState->backTrackRing) );
					if ( pVadState->isNeedFlag )
					{
						clearBufRing( &(pVadState->backTrackFlag) );
					}
				}
			}
		}
	}
	else // in speech region
	{
		if ( ret == 1 )
		{
			if ( pVadState->speechWritePos < pVadState->speechFrameNum )
			{
				memmove( pVadState->pSpeechBuf + pVadState->speechWritePos * VAD_FRAME_LEN, pFrameBuf, VAD_FRAME_LEN * BYTES_PER_SAMPLE );
				if ( pVadState->isNeedFlag )
				{
					pVadState->pSpeechFlag[pVadState->speechWritePos] = flag;
				}
				(pVadState->speechWritePos)++;

				pVadState->validSpeechSamples += VAD_FRAME_LEN;
			}
			else
			{
				// utterance too long to be key word
				pVadState->isSpeechRegion = 0;
				pVadState->isTransition = 0;

				pVadState->smoothWinIdx   = 0;
				for ( i = 0; i < SMOOTH_WIN_SIZE; i++ )
				{
					pVadState->smoothWin[i] = 0;
				}

				pVadState->alertWinIdx = 0;
				for ( i = 0; i < ALERT_CONTEXT_SIZE; i++ )
				{
					pVadState->alertContextWin[i] = 0;
				}

				pVadState->speechWritePos     = pVadState->speechStartPos;
				pVadState->validSpeechSamples = 0;

				// AUDLOG( "clear back track ring\n" );
				clearBufRing( &(pVadState->backTrackRing) );
				if ( pVadState->isNeedFlag )
				{
					clearBufRing( &(pVadState->backTrackFlag) );
				}
			}
		}
		else // ret == 0
		{
			if ( pVadState->speechWritePos < pVadState->speechFrameNum )
			{
				memmove( pVadState->pSpeechBuf + pVadState->speechWritePos * VAD_FRAME_LEN, pFrameBuf, VAD_FRAME_LEN * BYTES_PER_SAMPLE );
				if ( pVadState->isNeedFlag )
				{
					pVadState->pSpeechFlag[pVadState->speechWritePos] = flag;
				}
				(pVadState->speechWritePos)++;

				pVadState->validSpeechSamples += VAD_FRAME_LEN;
			}

			if ( pVadState->isTransition == 0 )
			{
				pVadState->isTransition = 1;
			}
		}

		if ( pVadState->isTransition == 1 )
		{
			if ( ret == 1 )
			{
				pVadState->smoothWin[pVadState->smoothWinIdx] = 0;
			}
			else
			{
				pVadState->smoothWin[pVadState->smoothWinIdx] = 1;
			}
			pVadState->smoothWinIdx++;

			if ( pVadState->smoothWinIdx == SPEECH_TO_NONSPEECH_SMOOTH )
			{
				AUD_Int32s nonspeechCnt = 0;
				for ( i = 0; i < SPEECH_TO_NONSPEECH_SMOOTH; i++ )
				{
					nonspeechCnt += pVadState->smoothWin[i];
				}

				AUD_Int32s winTrail = 0;
				for ( i = SPEECH_TO_NONSPEECH_SMOOTH - 4; i < SPEECH_TO_NONSPEECH_SMOOTH; i++ )
				{
					winTrail += pVadState->smoothWin[i];
				}

				if ( nonspeechCnt >= SPEECH_TO_NONSPEECH_THRESHOLD && winTrail >= 2 )
				{
					// AUDLOG( "speech end\n" );
					for ( i = 0; i < ALERT_CONTEXT_SIZE; i++ )
					{
						pVadState->alertContextWin[i] = 0;
					}
					pVadState->alertWinIdx    = 0;
					pVadState->isSpeechRegion = 0;
					pVadState->isTailing      = 1;
				}
				else
				{
					// act as nothing happened
					// AUDLOG( "speech continue\n" );
					pVadState->smoothWinIdx = 0;
					for ( i = 0; i < SMOOTH_WIN_SIZE; i++ )
					{
						pVadState->smoothWin[i] = 0;
					}
					pVadState->isTransition = 0;
				}
			}
		}
	}

	return 0;
}

// energy based frame admission
AUD_Int32s sig_frameadmit( AUD_Int16s *pInBuf, AUD_Int32s inSampleNum, AUD_Int16s *pOutBuf )
{
	AUD_Int32s i, j;
	AUD_Int32s frameNum = inSampleNum / FRAME_STRIDE;

	AUD_Double *pEnergy = (AUD_Double*)calloc( frameNum * sizeof( AUD_Double ), 1 );
	AUD_ASSERT( pEnergy );

	AUD_Int16s *pBuf     = NULL;
	AUD_Double maxEnergy = 0.;
	for ( i = 0; i * FRAME_STRIDE + FRAME_LEN < inSampleNum; i++ )
	{
		pBuf       = pInBuf + i * FRAME_STRIDE;
		pEnergy[i] = 0.;
		for ( j = 0; j < FRAME_LEN; j++ )
		{
			pEnergy[i] += pBuf[j] * pBuf[j];
		}
		if ( maxEnergy < pEnergy[i] )
		{
			maxEnergy = pEnergy[i];
		}
	}
	frameNum = i;

	AUD_Int16s *pValid        = pOutBuf;
	AUD_Int32s validSampleNum = 0;
	for ( i = 0; i < frameNum; i++ )
	{
		// take SNR >= 30dB after denoise
		if ( pEnergy[i] >= 0.001 * maxEnergy )
		{
			memmove( pValid, pInBuf + i * FRAME_STRIDE, FRAME_LEN * BYTES_PER_SAMPLE );
			validSampleNum += FRAME_LEN;
			pValid         += FRAME_LEN;
		}
	}

	// AUDLOG( "total frame: %d, valid frame: %d\n", frameNum, validSampleNum / FRAME_LEN );

	free( pEnergy );
	pEnergy = NULL;

	return validSampleNum;
}
