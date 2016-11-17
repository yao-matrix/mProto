#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

#include "io/file/io.h"

static const char* keywords[] =
{
	"academy",
	"computer",
	"geniusbutton",
	"hamburger",
	"hellobluegenie",
	"hellointel",
	"heyvlingo",
	"listentome",
	"original",
	"startlistening",
	"wakeup",
};

static int keywordNum = sizeof(keywords) / sizeof(keywords[0]);
static int speakerNum = 11;

#define ENROLLMENT_NUM  3

static void split( int x[], int left, int right, int *split_point )
{
	int split_data = x[left];
	int i;

	for ( *split_point = left, i = left + 1; i <= right; i++ )
	{
		if ( x[i] < split_data )
		{
			(*split_point)++;
			SWAPINT( x[*split_point], x[i] );
		}
	}
	SWAPINT( x[left], x[*split_point] );
}

static int median( int x[], int n )
{
	int left  = 0;
	int right = n - 1;
	int mid   = (left + right) / 2;
	int  split_point;

	while ( 1 )
	{
		/* loop until we hit the mid */
		split( x, left, right, &split_point );
		if ( split_point == mid )
		{
			break;
		}
		else if ( split_point > mid )
		{
			right = split_point - 1;
		}
		else
		{
			left  = split_point + 1; /* use right part */
		}
	}

	return x[mid];
}


AUD_Int32s train_keyword_hmm_multienroll( const AUD_Int8s *pKeywordPath )
{
	AUD_Error  error = AUD_ERROR_NONE;
	AUD_Int32s ret   = 0;

	// step 1: read garbage model from file
	void *hGarbageGmm = NULL;
	FILE *fpGarbage = fopen( (char*)WOV_UBM_GMMHMMMODEL_FILE, "rb" );
	if ( fpGarbage == NULL )
	{
		AUDLOG( "cannot open gmm model file: [%s]\n", WOV_UBM_GMMHMMMODEL_FILE );
		return AUD_ERROR_IOFAILED;
	}
	error = gmm_import( &hGarbageGmm, fpGarbage );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	fclose( fpGarbage );
	fpGarbage = NULL;

	// AUDLOG( "garbage GMM as:\n" );
	// gmm_show( hGarbageGmm );

	AUD_Int32s keywordID, speakerID;
	AUD_Int32s i = 0, j = 0, k = 0, m = 0, n = 0;

	for ( keywordID = 0; keywordID < keywordNum; keywordID++ )
	{
		// keyword feature extraction
		for ( speakerID = 1; speakerID <= speakerNum; speakerID++ )
		{
			AUD_Int32s sampleNum = 0;
			AUD_Int32s bufLen    = SAMPLE_RATE * BYTES_PER_SAMPLE * 10;
			AUD_Int16s *pBuf     = (AUD_Int16s*)calloc( bufLen, 1 );
			AUD_ASSERT( pBuf );

			AUD_Feature arKeywordFeature[ENROLLMENT_NUM];
			AUD_Int32s  *arClusterLabel[ENROLLMENT_NUM];
			AUD_Int32s  arStateNum[ENROLLMENT_NUM];
			AUD_Matrix  arIndexTable[ENROLLMENT_NUM];
			AUD_Matrix  arLlrTable[ENROLLMENT_NUM];

			for ( i = 0; i < ENROLLMENT_NUM; i++ )
			{
				void *hMfccHandle      = NULL;
				char keywordFile[256] = { 0, };

				snprintf( keywordFile, 256, "%s/%s/%d-%s-%d_16k.wav", pKeywordPath, keywords[keywordID], speakerID, keywords[keywordID], i + 1 );
				AUDLOG( "%s\n", keywordFile );

				sampleNum = readWavFromFile( (AUD_Int8s*)keywordFile, pBuf, bufLen );
				AUD_ASSERT( sampleNum > 0 );

				// pre-processing

				 // pre-emphasis
				sig_preemphasis( pBuf, pBuf, sampleNum );


				// calc framing number
				for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= sampleNum; j++ )
				{
					;
				}

				// XXX: select salient frames

				// extract MFCC feature
				arKeywordFeature[i].featureMatrix.rows     = j - MFCC_DELAY;
				arKeywordFeature[i].featureMatrix.cols     = MFCC_FEATDIM;
				arKeywordFeature[i].featureMatrix.dataType = AUD_DATATYPE_INT32S;
				ret = createMatrix( &(arKeywordFeature[i].featureMatrix) );
				AUD_ASSERT( ret == 0 );

				arKeywordFeature[i].featureNorm.len      = j - MFCC_DELAY;
				arKeywordFeature[i].featureNorm.dataType = AUD_DATATYPE_INT64S;
				ret = createVector( &(arKeywordFeature[i].featureNorm) );
				AUD_ASSERT( ret == 0 );

				error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				error = mfcc16s32s_calc( hMfccHandle, pBuf, sampleNum, &(arKeywordFeature[i]) );
				AUD_ASSERT( error == AUD_ERROR_NONE );


				// for each feature vector, get the bestN most likelihood component indices from GMM
				AUD_Vector compLlr;
				compLlr.len      = gmm_getmixnum( hGarbageGmm );
				compLlr.dataType = AUD_DATATYPE_DOUBLE;
				ret = createVector( &compLlr );
				AUD_ASSERT( ret == 0 );

				arIndexTable[i].rows     = arKeywordFeature[i].featureMatrix.rows ;
				arIndexTable[i].cols     = WOV_KEYWORD_GMMMODEL_ORDER;
				arIndexTable[i].dataType = AUD_DATATYPE_INT32S;
				ret = createMatrix( &arIndexTable[i] );
				AUD_ASSERT( ret == 0 );

				arLlrTable[i].rows     = arKeywordFeature[i].featureMatrix.rows;
				arLlrTable[i].cols     = WOV_KEYWORD_GMMMODEL_ORDER;
				arLlrTable[i].dataType = AUD_DATATYPE_DOUBLE;
				ret = createMatrix( &arLlrTable[i] );
				AUD_ASSERT( ret == 0 );

				AUD_Double totalLLR;

				for ( j = 0; j < arKeywordFeature[i].featureMatrix.rows; j++ )
				{
					totalLLR = gmm_llr( hGarbageGmm, &(arKeywordFeature[i].featureMatrix), j, &compLlr );

					// sort the bestN most likelihood
					AUD_Vector bestIndex;
					bestIndex.len       = WOV_KEYWORD_GMMMODEL_ORDER;
					bestIndex.dataType  = AUD_DATATYPE_INT32S;
					bestIndex.pInt32s   = arIndexTable[i].pInt32s + j * arIndexTable[i].cols;

					ret = sortVector( &compLlr, &bestIndex );
					AUD_ASSERT( ret == 0 );

					AUD_Double *pLLR   = arLlrTable[i].pDouble + j * arLlrTable[i].cols;
					for ( m = 0; m < bestIndex.len  ; m++ )
					{
						pLLR[m] = compLlr.pDouble[bestIndex.pInt32s[m]];
					}
				}

				ret = destroyVector( &compLlr );
				AUD_ASSERT( ret == 0 );

				showMatrix( &arIndexTable[i] );
				showMatrix( &arLlrTable[i] );

				// cluster GMM
				AUDLOG( "++++++++\n" );
				arClusterLabel[i] = (AUD_Int32s*)calloc( sizeof(AUD_Int32s) * arKeywordFeature[i].featureMatrix.rows, 1 );
				error = gmm_cluster( hGarbageGmm, &arIndexTable[i], WOV_GMM_CLUSTER_THRESHOLD, arClusterLabel[i] );
				AUD_ASSERT( error == AUD_ERROR_NONE );
				AUDLOG( "--------\n" );

				arStateNum[i] = arClusterLabel[i][arKeywordFeature[i].featureMatrix.rows - 1] + 1;
				// AUD_ASSERT( arStateNum[i] > 5 );

				error = mfcc16s32s_deinit( &hMfccHandle );
				AUD_ASSERT( error == AUD_ERROR_NONE );
			}
			free( pBuf );
			pBuf   = NULL;
			bufLen = 0;

			// select and build state
			AUD_Int32s  medianStateNum = -1;
			medianStateNum = median( arStateNum, ENROLLMENT_NUM );

			AUDLOG( "Start reclustering\n" );
			// adjust to let all HMM same state number
			for ( i = 0; i < ENROLLMENT_NUM; i++ )
			{
				AUD_Double step = 0.;
				if ( arStateNum[i] > medianStateNum )
				{
					step = 0.2;
				}
				else if ( arStateNum[i] < medianStateNum )
				{
					step = -0.2;
				}
				else
				{
					continue;
				}

				AUD_Int32s cnt   = 100;
				AUD_Double thesh = WOV_GMM_CLUSTER_THRESHOLD + step;
				while ( cnt >= 0 )
				{
					error = gmm_cluster( hGarbageGmm, &arIndexTable[i], thesh, arClusterLabel[i] );
					AUD_ASSERT( error == AUD_ERROR_NONE );

					arStateNum[i] = arClusterLabel[i][arKeywordFeature[i].featureMatrix.rows - 1];
					if ( arStateNum[i] == medianStateNum )
					{
						break;
					}
					else if ( arStateNum[i] > medianStateNum )
					{
						if ( step < 0. )
						{
							AUDLOG( "jitter\n" );
							AUD_ASSERT( 0 );
						}
						else
						{
							thesh += step;
						}
					}
					else
					{
						if ( step > 0. )
						{
							AUDLOG( "jitter\n" );
							AUD_ASSERT( 0 );
						}
						else
						{
							thesh += step;
						}
					}
					cnt--;
				}

				if ( cnt < 0 )
				{
					AUD_ASSERT( 0 );
				}
			}

			// sift GMM probabilities
			void **phKeywordGmms = (void**)calloc( sizeof(void*) * medianStateNum, 1 );
			AUD_ASSERT( phKeywordGmms );

			AUD_Vector indexVector;
			indexVector.len      = WOV_KEYWORD_GMMMODEL_ORDER;
			indexVector.dataType = AUD_DATATYPE_INT32S;
			ret = createVector( &indexVector );
			AUD_ASSERT( ret == 0 );

			AUD_Vector llrVector;
			llrVector.len      = WOV_KEYWORD_GMMMODEL_ORDER;
			llrVector.dataType = AUD_DATATYPE_DOUBLE;
			ret = createVector( &llrVector );
			AUD_ASSERT( ret == 0 );

			AUD_Int32s arStart[ENROLLMENT_NUM] = { 0, }, arEnd[ENROLLMENT_NUM] = { 0, };
			for ( i = 0; i < medianStateNum; i++ )
			{
				for ( j = 0; j < indexVector.len; j++ )
				{
					indexVector.pInt32s[j] = -1;
					llrVector.pInt32s[j]   = 1.;
				}

				for ( j = 0; j < ENROLLMENT_NUM; j++ )
				{
					while ( arClusterLabel[j][arStart[j]] == -1 )
					{
						arStart[j]++;
					}

					for ( k = arStart[j]; k < arKeywordFeature[j].featureMatrix.rows; k++ )
					{
						if ( arClusterLabel[j][k] != i )
						{
							break;
						}
					}
					arEnd[j] = k;

					for ( k = arStart[j] * arLlrTable[j].cols; k < arEnd[j] * arLlrTable[j].cols; k++ )
					{
						for ( m = 0; m < indexVector.len; m++ )
						{
							if ( arLlrTable[j].pDouble[k] == llrVector.pDouble[m] &&
							     arIndexTable[j].pInt32s[k] == indexVector.pInt32s[m] )
							{
								break;
							}
							else if ( indexVector.pInt32s[m] == -1 || arLlrTable[j].pDouble[k] > llrVector.pDouble[m] )
							{
								for ( n = indexVector.len - 1; n > m ; n-- )
								{
									indexVector.pInt32s[n] = indexVector.pInt32s[n - 1];
									llrVector.pDouble[n]   = llrVector.pDouble[n - 1];
								}
								indexVector.pInt32s[m] = arIndexTable[j].pInt32s[k];
								llrVector.pDouble[m]   = arLlrTable[j].pDouble[k];
								break;
							}
						}
					}
					arStart[j] = arEnd[j];
				}

				// AUDLOG( "Final GMM indices for state[%d]:\n", i );
				// showVector( &indexVector );

				AUD_Int8s gmmName[256] = { 0, };
				snprintf( (char*)gmmName, 256, "state%d", i );
				error = gmm_select( &phKeywordGmms[i], hGarbageGmm, &indexVector, 0, gmmName );
				AUD_ASSERT( error == AUD_ERROR_NONE );
			}

			for ( i = 0; i < ENROLLMENT_NUM; i++ )
			{
				ret = destroyMatrix( &arIndexTable[i] );
				AUD_ASSERT( ret == 0 );

				ret = destroyMatrix( &arLlrTable[i] );
				AUD_ASSERT( ret == 0 );

				free( arClusterLabel[i] );
				arClusterLabel[i] = NULL;
			}

			ret = destroyVector( &indexVector );
			AUD_ASSERT( ret == 0 );

			ret = destroyVector( &llrVector );
			AUD_ASSERT( ret == 0 );

			// generate keyword model by Baum-Welch algorithm
			AUD_Vector pi;
			pi.len      = medianStateNum;
			pi.dataType = AUD_DATATYPE_DOUBLE;
			ret = createVector( &pi );
			AUD_ASSERT( ret == 0 );

			pi.pDouble[0] = 1.0f;
			void *hKeywordHmm   = NULL;
			error = gmmhmm_init( &hKeywordHmm, medianStateNum, &pi, phKeywordGmms );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			error = gmmhmm_learn( hKeywordHmm, arKeywordFeature, ENROLLMENT_NUM, 0.001 );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			// export model to file
			char hmmName[256] = { 0, };
			snprintf( hmmName, 256, "%s/%d-%s.hmm", WOV_KEYWORD_GMMHMMMODEL_DIR, speakerID, keywords[keywordID] );
			error = gmmhmm_export( hKeywordHmm, (AUD_Int8s*)hmmName );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			// gmmhmm_show( hKeywordHmm );

			// clean field
			for ( i = 0; i < ENROLLMENT_NUM; i++ )
			{
				ret = destroyMatrix( &(arKeywordFeature[i].featureMatrix) );
				AUD_ASSERT( ret == 0 );

				ret = destroyVector( &(arKeywordFeature[i].featureNorm) );
				AUD_ASSERT( ret == 0 );
			}

			error = gmm_free( &hGarbageGmm );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			for ( i = 0; i < medianStateNum; i++ )
			{
				error = gmm_free( &phKeywordGmms[i] );
				AUD_ASSERT( error == AUD_ERROR_NONE );
			}
			free( phKeywordGmms );
			phKeywordGmms = NULL;

			ret = destroyVector( &pi );
			AUD_ASSERT( ret == 0 );

			error = gmmhmm_free( &hKeywordHmm );
			AUD_ASSERT( error == AUD_ERROR_NONE );
		}
	}

	return 0;
}
 
int train_hmm_me()
{
	AUD_Int8s  filePath[256] = { 0, };
	AUD_Int32s data;

	setbuf( stdout, NULL );
	setbuf( stdin, NULL );
	AUDLOG( "pls give training wav stream's folder path:\n" );
	filePath[0] = '\0';
	data = scanf( "%s", filePath );
	AUDLOG( "training folder path is: %s\n", filePath );

	train_keyword_hmm_multienroll( (const AUD_Int8s*)filePath );

	AUDLOG( "HMM multi-enroll training done\n" );

	return 0;
}

