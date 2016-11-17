#include <stdlib.h>
#include <memory.h>

#include <cv.h>
#include <ml.h>

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"

#include "gmm.h"

using namespace std;
using namespace cv;

AUD_Error gmm_train( void **phGmmHandle, AUD_Matrix *pSamples, AUD_Int8s *pGmmName, AUD_Int32s numMix, AUD_Int32s maxIter, AUD_Double minEps )
{
    GmmModel   *pState = NULL;
    AUD_Int32s cvDataType;
    AUD_Int32s ret;

    pState = (GmmModel*)calloc( sizeof(GmmModel), 1 );
    if ( pState == NULL )
    {
        *phGmmHandle = NULL;
        return AUD_ERROR_OUTOFMEMORY;
    }

    snprintf( (char*)pState->arGmmName, MAX_GMMNAME_LENGTH, "%s", (char*)pGmmName );

    switch ( pSamples->dataType )
    {
        case AUD_DATATYPE_INT16S:
            cvDataType = CV_16SC1;
            break;

        case AUD_DATATYPE_INT32S:
            cvDataType = CV_32SC1;
            break;

        case AUD_DATATYPE_DOUBLE:
            cvDataType = CV_64FC1;
            break;

        default:
            AUD_ASSERT( 0 );
            break;
    }

    // XXX: make assumption all types of pointer has same byte length here
    Mat cvSampleMatrix = Mat( pSamples->rows, pSamples->cols, cvDataType, (void*)(pSamples->pInt32s) );

    // Mat cvFloatSample( pSamples->rows, pSamples->cols, CV_32FC1, Scalar::all( 0.0 ) );

    // cvSampleMatrix.convertTo( cvFloatSample, CV_32FC1 );

    cv::EM em( numMix, EM::COV_MAT_DIAGONAL, TermCriteria( (minEps > 0) ? TermCriteria::COUNT + TermCriteria::EPS : TermCriteria::COUNT, maxIter, (minEps > 0) ? minEps : FLT_EPSILON ) );

    em.train( cvSampleMatrix, noArray(), noArray(), noArray() );

    // get learned gmm parameters
    Mat         cvWeights;
    Mat         cvMeans;
    vector<Mat> cvCovs;

    cvWeights = em.get< Mat >( "weights" );
    cvMeans   = em.get< Mat >( "means" );
    cvCovs    = em.get< vector<Mat> >( "covs" );

    AUDLOG( "EM algorithm clusters: %d, feature vector length: %d\n", cvMeans.rows, cvMeans.cols );

    AUD_Int32s i, j, k;
    AUD_Int32s *indexTable = (AUD_Int32s*)malloc( cvMeans.rows * sizeof(AUD_Int32s) );
    for ( i = 0; i < cvMeans.rows; i++ )
    {
        indexTable[i] = -1;
    }

#if 0

    // copy parameters to local handle
    for ( k = 0; k < cvMeans.rows; k++ )
    {
        indexTable[k] = k;
    }

#else

    k = 0;
    // select well discriminative gaussian components
    for ( i = 0; i < cvMeans.rows; i++ )
    {
        // calc trace of covariance matrix
        AUD_Double trace      = 0.;
        AUD_Bool   isSingular = AUD_FALSE;
        for ( j = 0; j < cvCovs[i].rows; j++ )
        {
            if ( cvCovs[i].at<double>( j, j ) < 0.5 && cvCovs[i].at<double>( j, j ) >= 0 )
            {
                isSingular = AUD_TRUE;
                break;
                // cvCovs[i].at<double>( j, j ) = 1.;
            }
            trace += sqrt( cvCovs[i].at<double>( j, j ) );
        }

        if ( isSingular == AUD_TRUE )
        {
            // AUD_ECHO
            continue;
        }
        else if ( trace > cvCovs[i].rows * MAX_INT32S / 12. )
        {
            // AUD_ECHO
            continue;
        }
        indexTable[k] = i;
        k++;
    }

#endif

    AUDLOG( "selected Gaussian Components: %d\n", k );

    pState->numMix  = k;
    pState->width   = pSamples->cols;
    pState->step    = pState->width;

    pState->means.rows     = pState->numMix;
    pState->means.cols     = pState->step;
    pState->means.dataType = AUD_DATATYPE_INT32S;
    ret = createMatrix( &(pState->means) );
    AUD_ASSERT( ret == 0 );

    pState->cvars.rows     = pState->numMix;
    pState->cvars.cols     = pState->step;
    pState->cvars.dataType = AUD_DATATYPE_INT64S;
    ret = createMatrix( &(pState->cvars) );
    AUD_ASSERT( ret == 0 );

    pState->weights.len      = pState->numMix;
    pState->weights.dataType = AUD_DATATYPE_DOUBLE;
    ret = createVector( &(pState->weights) );
    AUD_ASSERT( ret == 0 );

    for ( i = 0; i < k; i++ )
    {
        // weights
        *( pState->weights.pDouble + i ) = cvWeights.at<double>( indexTable[i] );
        // AUDLOG( "cvWeights.at<double>(%d): %f\n", i, cvWeights.at<double>( indexTable[i] ) );

        // means
        for ( j = 0; j < cvMeans.cols; j++ )
        {
            *(pState->means.pInt32s + i * pState->step + j) = (AUD_Int32s)round( cvMeans.at<double>( indexTable[i], j ) );
        }

        // cvars
        // AUDLOG( "cov matrix rows: %d, cols: %d\n", cvCovs[indexTable[i]].rows, cvCovs[indexTable[i]].cols );
        for ( j = 0; j < cvCovs[indexTable[i]].rows; j++ )
        {
            *(pState->cvars.pInt64s + i * pState->step + j) = (AUD_Int64s)round( cvCovs[indexTable[i]].at<double>( j, j ) );
        }
        // AUDLOG( "\n\n\n" );
    }

    *phGmmHandle = pState;

    return AUD_ERROR_NONE;
}
