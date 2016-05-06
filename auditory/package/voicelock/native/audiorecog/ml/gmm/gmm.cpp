/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "AudioDef.h"
#include "AudioUtil.h"
#include "AudioRecog.h"

#include "type/matrix.h"
#include "math/mmath.h"

#include "gmm.h"

/*
 * computes the log likelihood for one mixture component
 */
static void loggauss( AUD_Int32s *pSrc, AUD_Int32s *pMean, AUD_Int64s *pVar, AUD_Int32s featDim, AUD_Double *pVal, AUD_Double det )
{
    AUD_Double x0  = 0.;
    AUD_Double sum = 0.;
    AUD_Int32s j;

    for ( j = 0; j < featDim; j++ )
    {
        x0 = pSrc[j] - pMean[j];
        sum += x0 * x0 / pVar[j];
    }
    sum *= 0.5;

    *pVal = det - sum;

    return;
}

/*
 * computes log( determinant ) of each gaussian of GMM, where
 *
 *  log( determinant ) = log( weight ) - log( pow(2 * pi, k / 2) * pow( |var|, 0.5 ) )
 */
static void calcdet( AUD_Matrix *pCvar, AUD_Vector *pWeights, AUD_Vector *pDets )
{
    AUD_Int32s i, j;
    AUD_Double factor   = 0.;
    AUD_Int64s *pVarRow = NULL;

    for ( i    = 0; i < pWeights->len; i++ )
    {
        pVarRow = pCvar->pInt64s + i * pCvar->cols;
#if 1
        factor  = 0.;
        factor = log( pVarRow[0] / pow( 2.0, 32 ) );
        for ( j = 1; j < pCvar->cols; j++ )
        {
            factor += log( pVarRow[j] / pow( 2.0, 32 ) );
        }

        factor += pCvar->cols * log( TPI );
        factor /= 2.;

        (pDets->pDouble)[i] = log( pWeights->pDouble[i] ) - factor;
#else
        factor  = 1.;
        for ( j = 0; j < pCvar->cols; j++ )
        {
            factor *= TPI * pVarRow[j] / pow( 2.0, 32 );
        }

        factor = sqrt( factor );
        factor = pWeights->pDouble[i] / factor;

        (pDets->pDouble)[i] = log( factor );
#endif

        // AUDLOG( "det: %f \n", (pDets->pDouble)[i] );
    }
    // AUDLOG( "\n\n" );

    return;
}

AUD_Error gmm_show( void *hGmmHandle )
{
    AUD_ASSERT( hGmmHandle );

    GmmModel *pState = (GmmModel*)hGmmHandle;

#if 1
    char fName[256] = { 0, };
    snprintf( fName, 255, "./log/%s.txt", pState->arGmmName );
    FILE *fp = fopen( fName, "wb" );
    AUD_ASSERT( fp );
    #define AUDLOGF( fmt, ... )    fprintf( fp, fmt, ##__VA_ARGS__ )
#else
    #define AUDLOGF AUDLOG
#endif

    AUD_Int32s i, j;

    AUDLOGF( "\n" );
    AUDLOGF( "gmm model name             : [%s]\n", pState->arGmmName );
    AUDLOGF( "gmm model mixture number   : [%d]\n", pState->numMix );
    AUDLOGF( "gmm model feature dimension: [%d]\n", pState->width );
    AUDLOGF( "gmm model feature step     : [%d]\n", pState->step );
    AUDLOGF( "\n\n\n" );

    AUDLOGF( "gmm weights:\n" );
    for ( i = 0; i < pState->weights.len; i++ )
    {
        AUDLOGF( "%f, ", pState->weights.pDouble[i] );
    }
    AUDLOGF( "\n\n\n" );

    AUDLOGF( "gmm means:\n" );
    for ( i = 0; i < pState->means.rows; i++ )
    {
        for ( j = 0; j < pState->means.cols; j++ )
        {
            AUDLOGF( "%d, ", ( pState->means.pInt32s + i * pState->means.cols)[j] );
        }
        AUDLOGF( "\n\n" );
    }
    AUDLOGF( "\n\n\n" );

    AUDLOGF( "gmm cvars:\n" );
    for ( i = 0; i < pState->cvars.rows; i++ )
    {
        for ( j = 0; j < pState->cvars.cols; j++ )
        {
            AUDLOGF( "%lld, ", (pState->cvars.pInt64s + i * pState->cvars.cols)[j] );
        }
        AUDLOGF( "\n\n" );
    }
    AUDLOGF( "\n\n\n" );

    AUDLOGF( "gmm dets:\n" );
    for ( i = 0; i < pState->dets.len; i++ )
    {
        AUDLOGF( "%f, ", pState->dets.pDouble[i] );
    }
    AUDLOGF( "\n\n\n" );

#if 1
    fclose( fp );
    fp = NULL;
#endif

    return AUD_ERROR_NONE;
}

AUD_Error gmm_import( void **phGmmHandle, FILE *fp )
{
    AUD_ASSERT( phGmmHandle && fp );
    AUD_ASSERT( *phGmmHandle == NULL );

    GmmModel *pState = NULL;

    pState = (GmmModel*)calloc( sizeof(GmmModel), 1 );
    if ( pState == NULL )
    {
        *phGmmHandle = NULL;
        return AUD_ERROR_OUTOFMEMORY;
    }

    AUD_Int32s data, ret;

    // read gmm name
    data = fread( pState->arGmmName, 1, MAX_GMMNAME_LENGTH, fp );

    // read mixture num
    data = fread( &(pState->numMix), sizeof(AUD_Int32s), 1, fp );
    AUD_ASSERT( pState->numMix > 0 && pState->numMix <= 65536 );

    // read feature dimension
    data = fread( &(pState->width), sizeof(AUD_Int32s), 1, fp );
    AUD_ASSERT( pState->width > 0 && pState->width <= 65536 );

    // read mean & covariance step
    data = fread( &(pState->step), sizeof(AUD_Int32s), 1, fp );
    AUD_ASSERT( pState->step > 0 && pState->step <= 65536 );

    AUD_ASSERT( pState->step >= pState->width );

    // read weights
    pState->weights.len      = pState->numMix;
    pState->weights.dataType = AUD_DATATYPE_DOUBLE;
    ret = createVector( &(pState->weights) );
    AUD_ASSERT( ret == 0 );
    data = fread( pState->weights.pDouble, sizeof(AUD_Double), pState->weights.len, fp );

    // read means
    pState->means.rows     = pState->numMix;
    pState->means.cols     = pState->step;
    pState->means.dataType = AUD_DATATYPE_INT32S;
    ret = createMatrix( &(pState->means) );
    AUD_ASSERT( ret == 0 );
    data = fread( pState->means.pInt32s, sizeof(AUD_Int32s), pState->means.rows * pState->means.cols, fp );

    // read diagonal covariance
    pState->cvars.rows     = pState->numMix;
    pState->cvars.cols     = pState->step;
    pState->cvars.dataType = AUD_DATATYPE_INT64S;
    ret = createMatrix( &(pState->cvars) );
    AUD_ASSERT( ret == 0 );
    data = fread( pState->cvars.pInt64s, sizeof(AUD_Int64s), pState->cvars.rows * pState->cvars.cols, fp );

    // calc dets
    pState->dets.len      = pState->numMix;
    pState->dets.dataType = AUD_DATATYPE_DOUBLE;
    ret = createVector( &(pState->dets) );
    AUD_ASSERT( ret == 0 );
    calcdet( &(pState->cvars), &(pState->weights), &(pState->dets) );

    *phGmmHandle = pState;

    return AUD_ERROR_NONE;
}


AUD_Error gmm_init( void **phGmmHandle, AUD_Int8s *pModelName, AUD_Int32s numMix, AUD_Int32s featDim,
                    AUD_Vector *pWeights, AUD_Matrix *pMeans, AUD_Matrix *pCvars )
{
    AUD_ASSERT( phGmmHandle && pModelName && pWeights && pMeans && pCvars );
    AUD_ASSERT( *phGmmHandle == NULL );

    AUDLOG( "init model: %s\n", pModelName );

    AUD_Int32s ret = 0;

    GmmModel *pState = NULL;
    pState = (GmmModel*)calloc( sizeof(GmmModel), 1 );
    if ( pState == NULL )
    {
        *phGmmHandle = NULL;
        return AUD_ERROR_OUTOFMEMORY;
    }

    strncpy( (char*)pState->arGmmName, (char*)pModelName, MAX_GMMNAME_LENGTH - 1 );
    pState->arGmmName[strlen( (char*)pModelName )] = '\0';

    pState->numMix  = numMix;
    pState->width   = featDim;
    pState->step    = pState->width;

    pState->means.rows     = pState->numMix;
    pState->means.cols     = pState->step;
    pState->means.dataType = AUD_DATATYPE_INT32S;
    ret = createMatrix( &(pState->means) );
    AUD_ASSERT( ret == 0 );
    if ( pMeans != NULL )
    {
        ret = cloneMatrix( &(pState->means), pMeans );
        AUD_ASSERT( ret == 0 );
    }

    pState->cvars.rows     = pState->numMix;
    pState->cvars.cols     = pState->step;
    pState->cvars.dataType = AUD_DATATYPE_INT64S;
    ret = createMatrix( &(pState->cvars) );
    AUD_ASSERT( ret == 0 );
    if ( pCvars != NULL )
    {
        ret = cloneMatrix( &(pState->cvars), pCvars );
        AUD_ASSERT( ret == 0 );
    }

    pState->weights.len      = pState->numMix;
    pState->weights.dataType = AUD_DATATYPE_DOUBLE;
    ret = createVector( &(pState->weights) );
    AUD_ASSERT( ret == 0 );
    if ( pWeights != NULL )
    {
        ret = cloneVector( &(pState->weights), pWeights );
        AUD_ASSERT( ret == 0 );
    }

    pState->dets.len      = pState->numMix;
    pState->dets.dataType = AUD_DATATYPE_DOUBLE;
    ret = createVector( &(pState->dets) );
    AUD_ASSERT( ret == 0 );
    calcdet( &(pState->cvars), &(pState->weights), &(pState->dets) );

    *phGmmHandle = pState;

    return AUD_ERROR_NONE;
}

AUD_Error gmm_clone( void **phGmmHandle, void *hSrcGmmHandle, AUD_Int8s *pName )
{
    AUD_ASSERT( phGmmHandle && hSrcGmmHandle );
    AUD_ASSERT( *phGmmHandle == NULL );

    AUD_Int32s ret = 0;

    GmmModel *pState = NULL;
    pState = (GmmModel*)calloc( sizeof(GmmModel), 1 );
    if ( pState == NULL )
    {
        *phGmmHandle = NULL;
        return AUD_ERROR_OUTOFMEMORY;
    }

    GmmModel *pSrcModel = (GmmModel*)hSrcGmmHandle;

    if ( pName == NULL )
    {
        snprintf( (char*)pState->arGmmName, MAX_GMMNAME_LENGTH, "%s", (char*)pSrcModel->arGmmName );
    }
    else
    {
        snprintf( (char*)pState->arGmmName, MAX_GMMNAME_LENGTH, "%s", (char*)pName );
    }

    pState->numMix  = pSrcModel->numMix;
    pState->width   = pSrcModel->width;
    pState->step    = pState->width;

    pState->means.rows     = pState->numMix;
    pState->means.cols     = pState->step;
    pState->means.dataType = AUD_DATATYPE_INT32S;
    ret = createMatrix( &(pState->means) );
    AUD_ASSERT( ret == 0 );
    ret = cloneMatrix( &(pState->means), &(pSrcModel->means) );
    AUD_ASSERT( ret == 0 );

    pState->cvars.rows     = pState->numMix;
    pState->cvars.cols     = pState->step;
    pState->cvars.dataType = AUD_DATATYPE_INT64S;
    ret = createMatrix( &(pState->cvars) );
    AUD_ASSERT( ret == 0 );
    ret = cloneMatrix( &(pState->cvars), &(pSrcModel->cvars) );
    AUD_ASSERT( ret == 0 );

    pState->weights.len      = pState->numMix;
    pState->weights.dataType = AUD_DATATYPE_DOUBLE;
    ret = createVector( &(pState->weights) );
    AUD_ASSERT( ret == 0 );
    ret = cloneVector( &(pState->weights), &(pSrcModel->weights) );
    AUD_ASSERT( ret == 0 );

    pState->dets.len      = pState->numMix;
    pState->dets.dataType = AUD_DATATYPE_DOUBLE;
    ret = createVector( &(pState->dets) );
    AUD_ASSERT( ret == 0 );
    ret = cloneVector( &(pState->dets), &(pSrcModel->dets) );
    AUD_ASSERT( ret == 0 );

    *phGmmHandle = pState;

    return AUD_ERROR_NONE;
}

AUD_Error gmm_mix( void **phGmmHandle, void *hSrcGmmHandle1, void *hSrcGmmHandle2, AUD_Int8s *pName )
{
    AUD_ASSERT( phGmmHandle && hSrcGmmHandle1 && hSrcGmmHandle2 && pName );
    AUD_ASSERT( *phGmmHandle == NULL );

    AUD_Int32s ret = 0, i = 0;

    GmmModel *pState = NULL;
    pState = (GmmModel*)calloc( sizeof(GmmModel), 1 );
    if ( pState == NULL )
    {
        *phGmmHandle = NULL;
        return AUD_ERROR_OUTOFMEMORY;
    }

    GmmModel *pSrcModel1 = (GmmModel*)hSrcGmmHandle1;
    GmmModel *pSrcModel2 = (GmmModel*)hSrcGmmHandle2;

    AUD_ASSERT( pSrcModel1->width == pSrcModel2->width );

    snprintf( (char*)pState->arGmmName, MAX_GMMNAME_LENGTH, "%s", (char*)pName );

    pState->numMix  = pSrcModel1->numMix + pSrcModel2->numMix;
    pState->width   = pSrcModel1->width;
    pState->step    = pState->width;

    pState->means.rows     = pState->numMix;
    pState->means.cols     = pState->step;
    pState->means.dataType = AUD_DATATYPE_INT32S;
    ret = createMatrix( &(pState->means) );
    AUD_ASSERT( ret == 0 );

    ret = mergeMatrix( &(pState->means),  &(pSrcModel1->means), &(pSrcModel2->means) );
    AUD_ASSERT( ret == 0 );

    pState->cvars.rows     = pState->numMix;
    pState->cvars.cols     = pState->step;
    pState->cvars.dataType = AUD_DATATYPE_INT64S;
    ret = createMatrix( &(pState->cvars) );
    AUD_ASSERT( ret == 0 );

    ret = mergeMatrix( &(pState->cvars),  &(pSrcModel1->cvars), &(pSrcModel2->cvars) );
    AUD_ASSERT( ret == 0 );

    pState->weights.len      = pState->numMix;
    pState->weights.dataType = AUD_DATATYPE_DOUBLE;
    ret = createVector( &(pState->weights) );
    AUD_ASSERT( ret == 0 );

    AUD_Double *pDst1 = pState->weights.pDouble;
    AUD_Double *pDst2 = pState->weights.pDouble + pSrcModel1->weights.len;
    for ( i = 0; i < pSrcModel1->weights.len; i++ )
    {
        pDst1[i] = (pSrcModel1->weights.pDouble)[i] / 2.;
    }
    for ( i = 0; i < pSrcModel2->weights.len; i++ )
    {
        pDst2[i] = (pSrcModel2->weights.pDouble)[i] / 2.;
    }

    *phGmmHandle = pState;

    return AUD_ERROR_NONE;
}

AUD_Error gmm_select( void **phGmmHandle, void *hSrcGmmHandle, AUD_Vector *pIndex, AUD_Int32s selectMode, AUD_Int8s *pName )
{
    AUD_ASSERT( phGmmHandle && hSrcGmmHandle && pIndex && pName );
    AUD_ASSERT( pIndex->dataType == AUD_DATATYPE_INT32S && pIndex->len > 0 );
    AUD_ASSERT( *phGmmHandle == NULL );

    AUD_Int32s ret = 0, i;

    GmmModel *pState = NULL;
    pState = (GmmModel*)calloc( sizeof(GmmModel), 1 );
    if ( pState == NULL )
    {
        *phGmmHandle = NULL;
        return AUD_ERROR_OUTOFMEMORY;
    }

    GmmModel *pSrcModel = (GmmModel*)hSrcGmmHandle;

    snprintf( (char*)pState->arGmmName, MAX_GMMNAME_LENGTH, "%s", (char*)pName );

    AUD_Int8s *pValidFlag = (AUD_Int8s*)malloc( pSrcModel->numMix );
    AUD_ASSERT( pValidFlag );

    if ( selectMode & GMM_INVERTSELECT_MASK )
    {
        memset( pValidFlag, 1, pSrcModel->numMix );
        for ( i = 0; i < pIndex->len; i++ )
        {
            pValidFlag[pIndex->pInt32s[i]] = 0;
        }

        pState->numMix = pSrcModel->numMix - pIndex->len;
    }
    else
    {
        memset( pValidFlag, 0, pSrcModel->numMix );
        for ( i = 0; i < pIndex->len; i++ )
        {
            pValidFlag[pIndex->pInt32s[i]] = 1;
        }

        pState->numMix = pIndex->len;
    }

    pState->width   = pSrcModel->width;
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

    AUD_Double weightSum = 0.;
    AUD_Int32s selectRow = 0;

    for ( i = 0; i < pSrcModel->numMix && selectRow < pState->numMix; i++ )
    {
        if ( pValidFlag[i] == 1 )
        {
            ret = copyMatrixRow( &(pState->means), selectRow, &(pSrcModel->means), i );
            AUD_ASSERT( ret == 0 );
            ret = copyMatrixRow( &(pState->cvars), selectRow, &(pSrcModel->cvars), i );
            AUD_ASSERT( ret == 0 );

            pState->weights.pDouble[selectRow] = pSrcModel->weights.pDouble[i];

            weightSum += pSrcModel->weights.pDouble[i];
            selectRow++;
        }
    }

    for ( i = 0; i < pState->numMix; i++ )
    {
        pState->weights.pDouble[i] /= weightSum;
    }

    free( pValidFlag );
    pValidFlag = NULL;

    pState->dets.len      = pState->numMix;
    pState->dets.dataType = AUD_DATATYPE_DOUBLE;
    ret = createVector( &(pState->dets) );
    AUD_ASSERT( ret == 0 );

    calcdet( &(pState->cvars), &(pState->weights), &(pState->dets) );

    *phGmmHandle = pState;

    return AUD_ERROR_NONE;
}

AUD_Error gmm_free( void **phGmmHandle )
{
    AUD_ASSERT( phGmmHandle != NULL && *phGmmHandle != NULL );

    AUD_Int32s ret     = 0;
    GmmModel   *pState = (GmmModel*)(*phGmmHandle);

    ret = destroyMatrix( &(pState->means) );
    AUD_ASSERT( ret == 0 );

    ret = destroyMatrix( &(pState->cvars) );
    AUD_ASSERT( ret == 0 );

    ret = destroyVector( &(pState->weights) );
    AUD_ASSERT( ret == 0 );

    ret = destroyVector( &(pState->dets) );
    if ( ret == -1 )
    {
        AUDLOG( "this GMM has no determinant information\n" );
    }

    free( pState );
    pState = NULL;

    *phGmmHandle = NULL;

    return AUD_ERROR_NONE;
}

AUD_Error gmm_export( void *hGmmHandle, FILE *fp )
{
    AUD_ASSERT( hGmmHandle && fp );

    GmmModel *pState = (GmmModel*)hGmmHandle;

    // write gmm name
    (void)fwrite( pState->arGmmName, 1, MAX_GMMNAME_LENGTH, fp );

    // write num Mix
    (void)fwrite( &(pState->numMix), sizeof(AUD_Int32s), 1, fp );

    // write feature dimension
    (void)fwrite( &(pState->width), sizeof(AUD_Int32s), 1, fp );

    // write feature step
    (void)fwrite( &(pState->step), sizeof(AUD_Int32s), 1, fp );

    // write weights
    (void)fwrite( pState->weights.pDouble, sizeof(AUD_Double), pState->weights.len, fp );

    // write means
    (void)fwrite( pState->means.pInt32s, sizeof(AUD_Int32s), pState->means.rows * pState->means.cols, fp );

    // write diagonal covariance
    (void)fwrite( pState->cvars.pInt64s, sizeof(AUD_Int64s), pState->cvars.rows * pState->cvars.cols, fp );

    // AUDLOG( "exported GMM\n" );
    // gmm_show( pState );

    return AUD_ERROR_NONE;
}

/* compute LLR for Mixture of Gaussians */
AUD_Double gmm_llr( void *hGmmHandle, AUD_Matrix *pFeatMatrix, AUD_Int32s rowIndex, AUD_Vector *pComponentLLR )
{
    AUD_ASSERT( hGmmHandle && pFeatMatrix );
    AUD_ASSERT( pFeatMatrix->dataType == AUD_DATATYPE_INT32S );
    AUD_ASSERT( rowIndex < pFeatMatrix->rows );
    AUD_ASSERT( !pComponentLLR || ( pComponentLLR && pComponentLLR->dataType == AUD_DATATYPE_DOUBLE ) );

    GmmModel    *pState = (GmmModel*)hGmmHandle;
    AUD_Int32s  i;

    AUD_ASSERT( pFeatMatrix->cols == pState->width );

    AUD_Double  *llr = (AUD_Double*)calloc( pState->numMix * sizeof(AUD_Double), 1 );
    AUD_ASSERT( llr );

    AUD_Double  maxLLR = 0., totalLLR;

    if ( pState->dets.len <= 0 )
    {
        pState->dets.len      = pState->numMix;
        pState->dets.dataType = AUD_DATATYPE_DOUBLE;
        AUD_Int32s ret = createVector( &(pState->dets) );
        AUD_ASSERT( ret == 0 );

        calcdet( &(pState->cvars), &(pState->weights), &(pState->dets) );
    }


    for ( i = 0; i < pState->numMix; i++ )
    {
        loggauss( pFeatMatrix->pInt32s + rowIndex * pFeatMatrix->cols, pState->means.pInt32s + i * pState->step, pState->cvars.pInt64s + i * pState->step,
                  pFeatMatrix->cols, &llr[i], (pState->dets.pDouble)[i] );

        if ( pComponentLLR != NULL )
        {
            pComponentLLR->pDouble[i] = llr[i];
        }

        if ( i == 0 )
        {
            maxLLR = llr[i];
        }
        else
        {
            maxLLR = AUD_MAX( maxLLR, llr[i] );
        }
    }


    AUD_Double expDiffSum = 0.;
    for ( i = 0; i < pState->numMix; i++ )
    {
        expDiffSum += exp( llr[i] - maxLLR );
    }
    totalLLR = log( expDiffSum ) + maxLLR;

    free( llr );
    llr = NULL;

    if ( isnan( totalLLR ) || totalLLR < LOG_ZERO )
    {
        totalLLR = LOG_ZERO;
    }

    return totalLLR;
}


/* compute probability for Mixture of Gaussians */
AUD_Double gmm_prob( void *hGmmHandle, AUD_Matrix *pFeatMatrix, AUD_Int32s rowIndex, AUD_Vector *pComponentProb )
{
    AUD_ASSERT( hGmmHandle && pFeatMatrix );
    AUD_ASSERT( pFeatMatrix->dataType == AUD_DATATYPE_INT32S );
    AUD_ASSERT( rowIndex < pFeatMatrix->rows );
    AUD_ASSERT( !pComponentProb || ( pComponentProb && pComponentProb->dataType == AUD_DATATYPE_DOUBLE ) );

    GmmModel    *pState = (GmmModel*)hGmmHandle;
    AUD_Int32s  i;

    AUD_ASSERT( pFeatMatrix->cols == pState->width );

    AUD_Double  *llr   = (AUD_Double*)calloc( pState->numMix * sizeof(AUD_Double), 1 );
    AUD_ASSERT( llr );
    AUD_Double  maxLLR = 0.;

    for ( i = 0; i < pState->numMix; i++ )
    {
        loggauss( pFeatMatrix->pInt32s + rowIndex * pFeatMatrix->cols, pState->means.pInt32s + i * pState->step, pState->cvars.pInt64s + i * pState->step,
                  pState->width, &llr[i], (pState->dets.pDouble)[i] );

        if ( pComponentProb != NULL )
        {
            pComponentProb->pDouble[i] = exp( llr[i] );
        }

        if ( i == 0 )
        {
            maxLLR = llr[i];
        }
        else
        {
            maxLLR = AUD_MAX( maxLLR, llr[i] );
        }
    }

    AUD_Double expDiffSum = 0., totalLLR, prob;
    for ( i = 0; i < pState->numMix; i++ )
    {
        expDiffSum += exp( llr[i] - maxLLR );
    }
    totalLLR = log( expDiffSum ) + maxLLR;

    prob = exp( totalLLR );
    // AUDLOG( "totalLLR[%f], prob[%f]\n", totalLLR, prob );

    free( llr );
    llr = NULL;

    return prob;
}

/* Bayesian GMM adaptation */
AUD_Error gmm_adapt( void *hGmmHandle, AUD_Matrix *pAdaptData, bool &abortSignal )
{
    AUD_ASSERT( hGmmHandle && pAdaptData );
    AUD_ASSERT( pAdaptData->dataType == AUD_DATATYPE_INT32S );
    AUD_ASSERT( pAdaptData->rows > 0 );

    GmmModel   *pState = (GmmModel*)hGmmHandle;
    AUD_Int32s ret = 0;
    AUD_Int32s i, j, k, iter;

    AUD_ASSERT( pAdaptData->cols == pState->width );

    AUD_Double *pW = (AUD_Double*)calloc( pState->numMix * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pW );

    AUD_Matrix mean;
    mean.rows     = pState->numMix;
    mean.cols     = pState->width;
    mean.dataType = AUD_DATATYPE_DOUBLE;
    ret = createMatrix( &mean );
    AUD_ASSERT( ret == 0 );

    // second order moment
    // AUD_Matrix sndOrderMnt;
    // sndOrderMnt.rows     = pState->numMix;
    // sndOrderMnt.cols     = pState->width;
    // sndOrderMnt.dataType = AUD_DATATYPE_DOUBLE;
    // ret = createMatrix( &sndOrderMnt );
    // AUD_ASSERT( ret == 0 );

    AUD_Vector compProb;
    compProb.len      = pState->numMix;
    compProb.dataType = AUD_DATATYPE_DOUBLE;
    ret = createVector( &compProb );
    AUD_ASSERT( ret == 0 );

    AUD_Double gamma = 16.;

    for ( iter = 0; iter < 5; iter++ )
    {
        ret = cleanMatrix( &mean );
        AUD_ASSERT( ret == 0 );

        memset( pW, 0, pState->numMix * sizeof(AUD_Double) );

        for ( i = 0; i < pAdaptData->rows; i++ )
        {
            //Aborting Check Point
            if (abortSignal) {
                goto gmm_adapt_abort;
            }
            AUD_Double llr       = 0.;
            AUD_Int32s *pDataRow = pAdaptData->pInt32s + i * pAdaptData->cols;

            llr = gmm_llr( hGmmHandle, pAdaptData, i, &compProb );

            // AUDLOG( "component probability:\n" );
            for ( j = 0; j < mean.rows; j++ )
            {
                //Aborting Check Point
                if (abortSignal) {
                    goto gmm_adapt_abort;
                }
                AUD_Double *pMeanRow = mean.pDouble + j * mean.cols;
                AUD_Double prob;
                // AUD_Double *pSndOrderMntRow = sndOrderMnt.pDouble + j * sndOrderMnt.cols;

                prob = exp( compProb.pDouble[j] - llr );
                // AUDLOG( "%f, ", prob );

                pW[j] += prob;

                for ( k = 0; k < mean.cols; k++ )
                {
                    pMeanRow[k] += prob * pDataRow[k];
                    // pSndOrderMntRow[k] += prob * (AUD_Double)pDataRow[k] * (AUD_Double)pDataRow[k];

                    // AUD_ASSERT( pSndOrderMntRow[k] > 0 );
                }
            }
            // AUDLOG( "\n" );
        }

        // adaptation procedure
        // AUD_Double sumWeight = 0.;
        for ( j = 0; j < pState->numMix; j++ )
        {
            AUD_Double alpha = 1. / ( pW[j] + gamma );
            // AUDLOG( "alpha: %f\n", alpha );

            AUD_Double *pMeanRow        = mean.pDouble + j * mean.cols;
            // AUD_Double *pSndOrderMntRow = sndOrderMnt.pDouble + j * sndOrderMnt.cols;

            AUD_Int32s *pGmmMeanRow     = pState->means.pInt32s + j * pState->means.cols;
            // AUD_Int64s *pGmmVarRow      = pState->cvars.pInt64s + j * pState->cvars.cols;

            // pState->weights.pDouble[j] =( alpha * pW[j] / pAdaptData->rows + ( 1 - alpha ) * pState->weights.pDouble[j] ) * gamma;

            // sumWeight += pState->weights.pDouble[j];

            for ( k = 0; k < pState->width; k++ )
            {
                //Aborting Check Point
                if (abortSignal) {
                    goto gmm_adapt_abort;
                }
                AUD_Double tmpMean;
                // AUD_Double tmpVar;
                tmpMean = alpha * pMeanRow[k] + alpha * gamma * pGmmMeanRow[k];
                // tmpVar  = alpha * pSndOrderMntRow[k] - tmpMean * tmpMean + ( 1 - alpha ) * ( (AUD_Double)pGmmVarRow[k] ) + ( 1 - alpha ) * pGmmMeanRow[k] * pGmmMeanRow[k];

                // AUD_ASSERT( tmpVar > 0 );

                pGmmMeanRow[k] = (AUD_Int32s)tmpMean;
                // pGmmVarRow[k]  = (AUD_Int64s)tmpVar;

                // AUDLOG( "%f: %f : %d : %lld, ", tmpMean, tmpVar, pGmmMeanRow[k], pGmmVarRow[k] );
            }
            // AUDLOG( "\n" );
        }

        // re-normalize weightes
        // for ( j = 0; j < pState->numMix; j++ )
        // {
        //        pState->weights.pDouble[j] /= sumWeight;
        // }
    }

gmm_adapt_abort:

    free( pW );
    pW = NULL;

    ret = destroyVector( &compProb );
    AUD_ASSERT( ret == 0 );

    ret = destroyMatrix( &mean );
    AUD_ASSERT( ret == 0 );

    // ret = destroyMatrix( &sndOrderMnt );
    // AUD_ASSERT( ret == 0 );

    return AUD_ERROR_NONE;
}

AUD_Error gmm_getname( void *hGmmHandle, AUD_Int8s arGmmName[256] )
{
    AUD_ASSERT( hGmmHandle != NULL );

    GmmModel *pState = (GmmModel*)hGmmHandle;

    strcpy( (char*)arGmmName, (char*)(pState->arGmmName) );

    return AUD_ERROR_NONE;
}

AUD_Int32s gmm_getmixnum( void *hGmmHandle )
{
    AUD_ASSERT( hGmmHandle != NULL );

    GmmModel *pState = (GmmModel*)hGmmHandle;

    return pState->numMix;
}

