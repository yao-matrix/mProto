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


AUD_Int32s train_keyword_hmm_si( const AUD_Int8s *pKeywordPath )
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

	AUD_Int32s i = 0, j = 0, k = 0, m = 0, n = 0;

	entry *pEntry = NULL;
	dir   *pDir   = NULL;

	pDir = openDir( (const char*)pKeywordPath );
	if ( pDir == NULL )
	{
		AUDLOG( "cannot open folder: %s\n", pKeywordPath );
		return -1;
	}

	// get how many keyword files in directory
	int wavNum = 0;
	while ( ( pEntry = scanDir( pDir ) ) )
	{
		if ( strstr( pEntry->name, ".wav" ) == NULL )
		{
			continue;
		}
		wavNum++;
	}
	closeDir( pDir );
	pDir = NULL;

	AUDLOG( "total %d wav files\n", wavNum );
	AUD_ASSERT( wavNum > 0 );
	
	AUD_Feature *pKeywordFeats = (AUD_Feature*)calloc( wavNum * sizeof(AUD_Feature), 1 );
	AUD_ASSERT( pKeywordFeats );

	AUD_Int32s  **pClusterLabels = (AUD_Int32s**)calloc( wavNum * sizeof(AUD_Int32s*), 1 );
	AUD_ASSERT( pClusterLabels );

	AUD_Int32s  *pStateNum = (AUD_Int32s*)calloc( wavNum * sizeof(AUD_Int32s), 1 );
	AUD_ASSERT( pStateNum );

	AUD_Matrix  *pIndexTable = (AUD_Matrix*)calloc( wavNum * sizeof(AUD_Matrix), 1 );
	AUD_ASSERT( pIndexTable );

	AUD_Matrix  *pLlrTable = (AUD_Matrix*)calloc( wavNum * sizeof(AUD_Matrix), 1 );
	AUD_ASSERT( pLlrTable );

	AUD_Int32s bufLen = 0;
	AUD_Int16s *pBuf  = NULL;

	while ( ( pEntry = scanDir( pDir ) ) )
	{
		char        keywordFile[256] = { 0, };
		AUD_Summary fileSummary;
		AUD_Int32s  sampleNum        = 0;
		void        *hMfccHandle     = NULL;

		snprintf( keywordFile, 256, "%s/%s", pKeywordPath, pEntry->name );
		AUDLOG( "%s\n", keywordFile );

		ret = parseWavFromFile( (AUD_Int8s*)keywordFile, &fileSummary );
		if ( ret < 0 )
		{
			continue;
		}
		AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );

		bufLen = fileSummary.dataChunkBytes;
		pBuf  = (AUD_Int16s*)calloc( bufLen, 1 );
		AUD_ASSERT( pBuf );

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
		pKeywordFeats[i].featureMatrix.rows     = j - MFCC_DELAY;
		pKeywordFeats[i].featureMatrix.cols     = MFCC_FEATDIM;
		pKeywordFeats[i].featureMatrix.dataType = AUD_DATATYPE_INT32S;
		ret = createMatrix( &(pKeywordFeats[i].featureMatrix) );
		AUD_ASSERT( ret == 0 );

		pKeywordFeats[i].featureNorm.len      = j - MFCC_DELAY;
		pKeywordFeats[i].featureNorm.dataType = AUD_DATATYPE_INT64S;
		ret = createVector( &(pKeywordFeats[i].featureNorm) );
		AUD_ASSERT( ret == 0 );

		error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		error = mfcc16s32s_calc( hMfccHandle, pBuf, sampleNum, &(pKeywordFeats[i]) );
		AUD_ASSERT( error == AUD_ERROR_NONE );


		// for each feature vector, get the bestN most likelihood component indices from GMM
		AUD_Vector compLlr;
		compLlr.len      = gmm_getmixnum( hGarbageGmm );
		compLlr.dataType = AUD_DATATYPE_DOUBLE;
		ret = createVector( &compLlr );
		AUD_ASSERT( ret == 0 );

		pIndexTable[i].rows     = pKeywordFeats[i].featureMatrix.rows ;
		pIndexTable[i].cols     = WOV_KEYWORD_GMMMODEL_ORDER;
		pIndexTable[i].dataType = AUD_DATATYPE_INT32S;
		ret = createMatrix( &pIndexTable[i] );
		AUD_ASSERT( ret == 0 );

		pLlrTable[i].rows     = pKeywordFeats[i].featureMatrix.rows;
		pLlrTable[i].cols     = WOV_KEYWORD_GMMMODEL_ORDER;
		pLlrTable[i].dataType = AUD_DATATYPE_DOUBLE;
		ret = createMatrix( &pLlrTable[i] );
		AUD_ASSERT( ret == 0 );

		AUD_Double totalLLR;

		for ( j = 0; j < pKeywordFeats[i].featureMatrix.rows; j++ )
		{
			totalLLR = gmm_llr( hGarbageGmm, &(pKeywordFeats[i].featureMatrix), j, &compLlr );

			// sort the bestN most likelihood
			AUD_Vector bestIndex;
			bestIndex.len           = WOV_KEYWORD_GMMMODEL_ORDER;
			bestIndex.dataType      = AUD_DATATYPE_INT32S;
			bestIndex.pInt32s = pIndexTable[i].pInt32s + j * pIndexTable[i].cols;

			ret = sortVector( &compLlr, &bestIndex );
			AUD_ASSERT( ret == 0 );

			AUD_Double *pLLR   = pLlrTable[i].pDouble + j * pLlrTable[i].cols;
			for ( m = 0; m < bestIndex.len  ; m++ )
			{
				pLLR[m] = compLlr.pDouble[bestIndex.pInt32s[m]];
			}
		}

		ret = destroyVector( &compLlr );
		AUD_ASSERT( ret == 0 );

		// cluster GMM
		AUDLOG( "++++++++\n" );
		pClusterLabels[i] = (AUD_Int32s*)calloc( sizeof(AUD_Int32s) * pKeywordFeats[i].featureMatrix.rows, 1 );
		error = gmm_cluster( hGarbageGmm, &pIndexTable[i], WOV_GMM_CLUSTER_THRESHOLD, pClusterLabels[i] );
		AUD_ASSERT( error == AUD_ERROR_NONE );
		AUDLOG( "--------\n" );

		pStateNum[i] = pClusterLabels[i][pKeywordFeats[i].featureMatrix.rows - 1];
		AUD_ASSERT( pStateNum[i] > 5 );

		error = mfcc16s32s_deinit( &hMfccHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		free( pBuf );
		pBuf   = NULL;
		bufLen = 0;
	}
	closeDir( pDir );
	pDir = NULL;

	// select and build state
	AUD_Int32s  medianStateNum = -1;
	medianStateNum = median( pStateNum, wavNum );

	// adjust to let all HMM same state number
	for ( i = 0; i < wavNum; i++ )
	{
		AUD_Double step = 0.;
		if ( pStateNum[i] > medianStateNum )
		{
			step = 0.2;
		}
		else if ( pStateNum[i] < medianStateNum )
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
			error = gmm_cluster( hGarbageGmm, &pIndexTable[i], thesh, pClusterLabels[i] );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			pStateNum[i] = pClusterLabels[i][pKeywordFeats[i].featureMatrix.rows - 1];
			if ( pStateNum[i] == medianStateNum )
			{
				break;
			}
			else if ( pStateNum[i] > medianStateNum )
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

	AUD_Int32s  *pStart = (AUD_Int32s*)calloc( wavNum * sizeof(AUD_Int32s), 1 );
	AUD_ASSERT( pStart );

	AUD_Int32s  *pEnd = (AUD_Int32s*)calloc( wavNum * sizeof(AUD_Int32s), 1 );
	AUD_ASSERT( pEnd );

	for ( i = 0; i < medianStateNum; i++ )
	{
		for ( j = 0; j < indexVector.len; j++ )
		{
			indexVector.pInt32s[j] = -1;
			llrVector.pInt32s[j]   = 1.;
		}

		for ( j = 0; j < wavNum; j++ )
		{
			for ( k = pStart[j]; k < pKeywordFeats[j].featureMatrix.rows; k++ )
			{
				if ( pClusterLabels[j][k] != i )
				{
					break;
				}
			}
			pEnd[j] = k;

			for ( k = pStart[j] * pLlrTable[j].cols; k < pEnd[j] * pLlrTable[j].cols; k++ )
			{
				for ( m = 0; m < indexVector.len; m++ )
				{
					if ( pLlrTable[j].pDouble[k] == llrVector.pDouble[m] &&
                         pIndexTable[j].pInt32s[k] == indexVector.pInt32s[m] )
					{
						break;
					}
					else if ( indexVector.pInt32s[m] == -1 || pLlrTable[j].pDouble[k] > llrVector.pDouble[m] )
					{
						for ( n = indexVector.len - 1; n > m ; n-- )
						{
							indexVector.pInt32s[n] = indexVector.pInt32s[n - 1];
							llrVector.pDouble[n]   = llrVector.pDouble[n - 1];
						}
						indexVector.pInt32s[m] = pIndexTable[j].pInt32s[k];
						llrVector.pDouble[m]   = pLlrTable[j].pDouble[k];
						break;
					}
				}
			}
			pStart[j] = pEnd[j];
		}

		// AUDLOG( "Final GMM indices for state[%d]:\n", i );
		// showVector( &indexVector );

		AUD_Int8s gmmName[256] = { 0, };
		snprintf( (char*)gmmName, 256, "state%d", i );
		error = gmm_select( &phKeywordGmms[i], hGarbageGmm, &indexVector, 0, gmmName );
		AUD_ASSERT( error == AUD_ERROR_NONE );
	}

	for ( i = 0; i < wavNum; i++ )
	{
		ret = destroyMatrix( &pIndexTable[i] );
		AUD_ASSERT( ret == 0 );

		ret = destroyMatrix( &pLlrTable[i] );
		AUD_ASSERT( ret == 0 );

		free( pClusterLabels[i] );
		pClusterLabels[i] = NULL;
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

	int slash = '/';
	char *ptr = strrchr( (char*)pKeywordPath, slash );
	ptr++;

	// export model to file
	char hmmName[256] = { 0, };
	snprintf( hmmName, 256, "%s/%s.hmm", WOV_KEYWORD_GMMHMMMODEL_DIR, ptr );
	error = gmmhmm_export( hKeywordHmm, (AUD_Int8s*)hmmName );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	// gmmhmm_show( hKeywordHmm );

	// clean field
	for ( i = 0; i < wavNum; i++ )
	{
		ret = destroyMatrix( &(pKeywordFeats[i].featureMatrix) );
		AUD_ASSERT( ret == 0 );

		ret = destroyVector( &(pKeywordFeats[i].featureNorm) );
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

	free( pStart );
	free( pEnd );
	free( pKeywordFeats );
	free( pClusterLabels );
	free( pStateNum );
	free( pIndexTable );
	free( pLlrTable );

	return 0;
}
 
int train_hmm_si()
{
	AUD_Int8s  filePath[256] = { 0, };
	AUD_Int32s data;

	setbuf( stdout, NULL );
	setbuf( stdin, NULL );
	AUDLOG( "pls give training wav stream's folder path:\n" );
	filePath[0] = '\0';
	data = scanf( "%s", filePath );
	AUDLOG( "training folder path is: %s\n", filePath );

	train_keyword_hmm_si( (const AUD_Int8s*)filePath );

	AUDLOG( "HMM speaker independent multi-enroll training done\n" );

	return 0;
}

