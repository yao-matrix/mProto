/*-------------------------------------------------------------------------
 * INTEL CONFIDENTIAL
 *
 * Copyright 2013 Intel Corporation All Rights Reserved.
 *
 * This source code and all documentation related to the source code
 * ("Material") contains trade secrets and proprietary and confidential
 * information of Intel and its suppliers and licensors. The Material is
 * deemed highly confidential, and is protected by worldwide copyright and
 * trade secret laws and treaty provisions. No part of the Material may be
 * used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you by
 * disclosure or delivery of the Materials, either expressly, by
 * implication, inducement, estoppel or otherwise. Any license under such
 * intellectual property rights must be express and approved by Intel in
 * writing.
 *-------------------------------------------------------------------------
 * Author: Sun, Taiyi (taiyi.sun@intel.com)
 * Create Time: 2013/04/18
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#include <vector>
#include <string>

#include <utils/Log.h>


#include "type/matrix.h"
#include "io/file/io.h"
#include "math/mmath.h"



#include "SpeakerVerification.h"

#undef LOG_TAG
#define LOG_TAG "LibSV"

//using namespace android;

#define SV_NORMSCORE_THRD 2.0
#define SV_SELFSCORE_THRD 0.5

#define TNORM_IMPOSTER_NUM 10
#define TOP_N              5


#define SV_LISTEN_MAX_FRAMENUM  64
#define SV_ENROLL_MAX_FRAMENUM    1875

#define LIBSV_ASSERT AUD_ASSERT

#define LIBSV_LOG(...) \
{\
    LOGD(__VA_ARGS__);\
}

SV::SV()
{
    libsv_state = LIBSV_STATE_NONE;
    inputLen = 0;
    abortPostEnroll = false;
    sv_result = LIBSV_CB_DEF;
#ifdef RECORD_PCM
    fp_pcm = NULL;
#endif
}

SV::~SV()
{
}

int SV::init_feat_extr(FrameFeatData &ffd)
{
    AUD_Error error = AUD_ERROR_NONE;
    int ret = 0;

    // init frame feature
    if (!ffd.featCreated) {
        ffd.frameFeat.featureMatrix.rows          = 1;
        ffd.frameFeat.featureMatrix.cols          = MFCC_FEATDIM;
        ffd.frameFeat.featureMatrix.dataType      = AUD_DATATYPE_INT32S;
        ret = createMatrix( &(ffd.frameFeat.featureMatrix) );
        LIBSV_ASSERT( ret == 0 );
        ffd.frameFeat.featureNorm.len             = 1;
        ffd.frameFeat.featureNorm.dataType        = AUD_DATATYPE_INT64S;
        ret = createVector( &(ffd.frameFeat.featureNorm) );
        LIBSV_ASSERT( ret == 0 );
        ffd.featCreated = true;
    }
    // init mfcc handle
    if (ffd.hFeatHandle == NULL) {
        error = mfcc16s32s_init( &(ffd.hFeatHandle), FRAME_LEN, WINDOW_TYPE,
            MFCC_ORDER, FRAME_LEN, SAMPLE_RATE, COMPRESS_TYPE );
        LIBSV_ASSERT( error == AUD_ERROR_NONE );
    }

    // init frame admission handle
    if (ffd.hVadHandle == NULL) {
        ret = sig_vadinit( &(ffd.hVadHandle), NULL, NULL, 0, -1, 6 );
        LIBSV_ASSERT( ret == 0 );
    }

    ffd.frameFeatNum = 0;
    return 0;
}

int SV::uninit_feat_extr(FrameFeatData &ffd)
{
    AUD_Error error = AUD_ERROR_NONE;
    int ret = 0;
    ffd.frameFeatNum = 0;
    if (ffd.hVadHandle != NULL) {
        sig_vaddeinit( &(ffd.hVadHandle) );
        ffd.hVadHandle = NULL;
    }
    // deinit mfcc handle
    if (ffd.hFeatHandle != NULL) {
        error = mfcc16s32s_deinit( &(ffd.hFeatHandle) );
        LIBSV_ASSERT( error == AUD_ERROR_NONE );
        ffd.hFeatHandle = NULL;
    }
    // destroy feature
    if (ffd.featCreated) {
        ret = destroyMatrix( &(ffd.frameFeat.featureMatrix) );
        LIBSV_ASSERT( ret == 0 );
        ret = destroyVector( &(ffd.frameFeat.featureNorm) );
        LIBSV_ASSERT( ret == 0 );
        ffd.featCreated = false;
    }
    return 0;
}

int SV::do_feat_extr(FrameFeatData &ffd, short *fbuf, short *pbuf)
{
    AUD_Error error = AUD_ERROR_NONE;
    int ret = 0;
    // frame admission
    AUD_Int16s flag;

    ret = sig_framecheck( ffd.hVadHandle, fbuf, &flag );
    if ( ret != 1 ) {
        return 0;
    }

    // Pre-processing: pre-emphasis
    sig_preemphasis( fbuf, pbuf, FRAME_LEN );

    // TODO: frame based de-noise here

    // calc MFCC feature
    error = mfcc16s32s_calc( ffd.hFeatHandle, pbuf, FRAME_LEN, &(ffd.frameFeat) );
    if ( error == AUD_ERROR_MOREDATA ) {
        return 0;
    }
    LIBSV_ASSERT( error == AUD_ERROR_NONE );
    ++ffd.frameFeatNum;
    return 1;
}

void* SV::load_model(const std::string &path)
{
    AUD_Error error = AUD_ERROR_NONE;
    void *handler = NULL;
    FILE *fpSpeaker = fopen( path.c_str(), "rb" );
    if ( fpSpeaker == NULL ) {
        LIBSV_LOG( "Cannot open speaker model file: [%s]\n", path.c_str() );
        return NULL;
    }
    error = gmm_import( &handler, fpSpeaker );
    LIBSV_ASSERT( error == AUD_ERROR_NONE );
    fclose( fpSpeaker );
    return handler;
}

int SV::init_model_set(ModelSet &ms)
{
    // load models
    ms.ubmModel.hModel = load_model(LIBSV_UBMMODEL_PATH);
    LIBSV_ASSERT( ms.ubmModel.hModel != NULL );

    if (ms.userModel.id >= 0) {
        ms.userModel.hModel = load_model(LIBSV_MODELS_DIR +
            std::string("/") + ms.userModel.name + ".gmm");
        LIBSV_ASSERT(ms.userModel.hModel != NULL);
    }

    for (unsigned i = 0; i < ms.modelVec.size(); ++i) {
        if (ms.modelVec[i].id < 0)
            continue;
        ms.modelVec[i].hModel = load_model(LIBSV_MODELS_DIR +
            std::string("/") + ms.modelVec[i].name + ".gmm");
        LIBSV_ASSERT(ms.modelVec[i].hModel != NULL);
    }
    return 0;
}

int SV::uninit_model_set(ModelSet &ms)
{
    AUD_Error error = AUD_ERROR_NONE;
    ModelInfo defmi;
    if (ms.ubmModel.hModel != NULL) {
        error = gmm_free( &(ms.ubmModel.hModel) );
        LIBSV_ASSERT( error == AUD_ERROR_NONE );
    }
    if (ms.userModel.hModel != NULL) {
        error = gmm_free( &(ms.userModel.hModel) );
        LIBSV_ASSERT( error == AUD_ERROR_NONE );
    }
    for (unsigned i = 0; i < ms.modelVec.size(); ++i) {
        if (ms.modelVec[i].hModel != NULL) {
            error = gmm_free( &(ms.modelVec[i].hModel) );
            LIBSV_ASSERT( error == AUD_ERROR_NONE );
        }
    }
    ms.modelVec.clear();
    ms.ubmModel = defmi;
    ms.userModel = defmi;
    return 0;
}

double SV::calc_aggr_score(void *hModel, AUD_Matrix *featMat,
        AUD_Vector *compLlr, AUD_Vector *bestIndex, bool sortComp)
{
    double score;
    gmm_llr( hModel, featMat, 0, compLlr );

    // get top N gaussian component
    if (sortComp) {
        int ret = sortVector( compLlr, bestIndex );
        LIBSV_ASSERT( ret == 0 );
    }

    double *pCompLlr = compLlr->pDouble;
    int *pBestIndex = bestIndex->pInt32s;
    score = pCompLlr[pBestIndex[0]];
    for( int i = 1; i < bestIndex->len; i++ )
    {
        score = logadd( score, pCompLlr[pBestIndex[i]] );
    }
    return score;
}

int SV::score_model_set(ModelSet &ms, FrameFeatData &ffd)
{
    int i = 0, j = 0;
    int ret   = 0;
    AUD_Error error = AUD_ERROR_NONE;

    // calc gmm scores
    AUD_Vector compLlr;
    compLlr.len      = gmm_getmixnum( ms.ubmModel.hModel );
    compLlr.dataType = AUD_DATATYPE_DOUBLE;
    ret = createVector( &compLlr );
    LIBSV_ASSERT( ret == 0 );

    AUD_Vector bestIndex;
    bestIndex.len      = TOP_N;
    bestIndex.dataType = AUD_DATATYPE_INT32S;
    ret = createVector( &bestIndex );
    LIBSV_ASSERT( ret == 0 );

    if (ms.ubmModel.hModel != NULL) {
        ms.ubmModel.score += calc_aggr_score(
            ms.ubmModel.hModel,
            &(ffd.frameFeat.featureMatrix),
            &compLlr, &bestIndex, true);
    }

    if (ms.userModel.hModel != NULL) {
        ms.userModel.score += calc_aggr_score(
            ms.userModel.hModel,
            &(ffd.frameFeat.featureMatrix),
            &compLlr, &bestIndex, false);
    }
    for (unsigned i = 0; i < ms.modelVec.size(); ++i) {
        if (ms.modelVec[i].hModel == NULL)
            continue;
        ms.modelVec[i].score += calc_aggr_score(
            ms.modelVec[i].hModel,
            &(ffd.frameFeat.featureMatrix),
            &compLlr, &bestIndex, false);
    }

    destroyVector( &bestIndex );
    destroyVector( &compLlr );
    return 0;
}

double SV::final_score_model_set(ModelSet &ms, FrameFeatData &ffd)
{
    double score;
    AUD_Double mean = 0., std = 0.;
    ms.userModel.finalScore =
        ( ms.userModel.score - ms.ubmModel.score ) / ffd.frameFeatNum;
    for ( unsigned i = 0; i < ms.modelVec.size(); ++i ) {
        ms.modelVec[i].finalScore =
            ( ms.modelVec[i].score - ms.ubmModel.score ) / ffd.frameFeatNum;
        mean += ms.modelVec[i].finalScore;
    }
    mean /= ms.modelVec.size();

    // T-Normalization
    for ( unsigned i = 0; i < ms.modelVec.size(); ++i ) {
        double diff = ms.modelVec[i].finalScore - mean;
        std += diff * diff;
    }
    std /= ms.modelVec.size();
    std = sqrt( std );

    score = (ms.userModel.finalScore - mean) / std;
#ifdef RECORD_PCM
    LOGD("%s scoring:", ms.userModel.name.c_str());
    LOGD("\tubm: %lf\n", ms.ubmModel.score);
    LOGD("\tuser %s: %lf, %lf", ms.userModel.name.c_str(), ms.userModel.score, ms.userModel.finalScore);
    for (unsigned i = 0; i < ms.modelVec.size(); ++i) {
        LOGD("\t%s: %lf, %lf", ms.modelVec[i].name.c_str(), ms.modelVec[i].score, ms.modelVec[i].finalScore);
    }
    LOGD("\tfinal score: %lf", score);
#endif
    return score;
}

int SV::check_filesys()
{
    if (access(LIBSV_SV_DIR, R_OK | W_OK | X_OK) != 0) {
        return -1;
    }
    if (access(LIBSV_MODELS_DIR, R_OK | W_OK | X_OK) != 0) {
        return -1;
    }
    if (access(LIBSV_RECORDS_DIR, R_OK | W_OK | X_OK) != 0) {
        return -1;
    }
    if (access(LIBSV_UBMMODEL_PATH, R_OK) != 0) {
        return -1;
    }
    if (access(LIBSV_IMPSTINFO_PATH, R_OK | W_OK) == 0) {
        if (impstInfo.load(LIBSV_IMPSTINFO_PATH))
            return 0;
        impstInfo.clear();
        if (!impstInfo.save(LIBSV_IMPSTINFO_PATH))
            return -1;
    }
    return 0;
}

int SV::copy_file(const std::string &dst, const std::string &src)
{
    int fdsrc, fddst;
    fdsrc = open(src.c_str(), O_RDONLY);
    if (fdsrc == -1) {
        return -1;
    }
    fddst = creat(dst.c_str(), O_WRONLY);
    if (fddst == -1) {
        close(fdsrc);
        return -1;
    }
    int s = sendfile(fddst, fdsrc, NULL, 1024);
    while (s > 0) {
        s = sendfile(fddst, fdsrc, NULL, 1024);
    }
    close(fdsrc);
    close(fddst);
    return 0;
}

int SV::copy_dir(const std::string &dst, const std::string &src)
{
    DIR *root = opendir(src.c_str());
    if (root == NULL)
        return -1;
    struct dirent *ent = readdir(root);
    while (ent != NULL) {
        std::string name(ent->d_name);
        if (name[0] == '.') {
            ent = readdir(root);
            continue;
        }
        if (ent->d_type == DT_REG) {
            if (copy_file(dst + "/" + name, src + "/" + name) != 0) {
                closedir(root);
                return -1;
            }
        } else if (ent->d_type == DT_DIR) {
            if (access((dst + "/" + name).c_str(), R_OK | W_OK | X_OK) != 0) {
                if (mkdir((dst + "/" + name).c_str(),
                    S_IRUSR | S_IWUSR | S_IXUSR |
                    S_IRGRP | S_IWGRP | S_IXGRP) != 0) {
                    closedir(root);
                    return -1;
                }
            }
            if (copy_dir(dst + "/" + name, src + "/" + name) != 0) {
                closedir(root);
                return -1;
            }
        }
        ent = readdir(root);
    }
    closedir(root);
    return 0;
}

int SV::init()
{
    if (libsv_state != LIBSV_STATE_NONE &&
        libsv_state != LIBSV_STATE_INIT) {
        LIBSV_LOG("%s: incorrect state", __func__);
        return -1;
    }

    if ( check_filesys() != 0 ) {
        if (copy_dir(LIBSV_ROOT_DIR, LIBSV_IMAGE_ROOT_DIR) != 0) {
            LIBSV_LOG("%s: copy_dir failed", __func__);
            return -1;
        }
        if ( check_filesys() != 0 ) {
            LIBSV_LOG("%s: check_filesys failed", __func__);
            return -1;
        }
    }

    if ( init_feat_extr(frameFeatData) != 0 ) {
        LIBSV_LOG("%s: failed on init_feat_extr", __func__);
        return -1;
    }

    sv_result = LIBSV_CB_DEF;
    libsv_state = LIBSV_STATE_INIT;
#ifdef RECORD_PCM
    fp_pcm = fopen("/sdcard/sv.pcm", "wb");
#endif
    return 0;
}

int SV::uninit(void)
{
    abortPostEnroll = false;
    uninit_feat_extr(frameFeatData);

    uninit_model_set(modelSet);

    libsv_state = LIBSV_STATE_NONE;
#ifdef RECORD_PCM
    if (fp_pcm != NULL) {
        fclose(fp_pcm);
        fp_pcm = NULL;
    }
#endif
    return 0;
}

int SV::start_listen(const char *user)
{
    if (libsv_state != LIBSV_STATE_INIT) {
        LIBSV_LOG("%s: incorrect state", __func__);
        return -1;
    }

    sv_result = LIBSV_CB_DEF;

    if (user == NULL) {
        LIBSV_LOG("%s: invalid arg", __func__);
        return -1;
    }

    char tmpbuf[256] = { 0 };
    strncpy(tmpbuf, user, 255);

    std::string userName = tmpbuf;
    int userIndex = impstInfo.index(userName);
    if (userIndex < 0) {
        LIBSV_LOG("%s: user %s not exist", __func__, tmpbuf);
        return -1;
    }

    inputLen = 0;
    frameFeatData.frameFeatNum = 0;

    modelSet.modelVec.clear();
    modelSet.userModel.id = userIndex;
    modelSet.userModel.name = impstInfo.row(userIndex).name;
    for (int i = 0; i < impstInfo.count(); ++i) {
        if ( impstInfo.item(userIndex, i).chosen ) {
            ModelInfo mi;
            mi.id = i;
            mi.name = impstInfo.row(i).name;
            modelSet.modelVec.push_back(mi);
        }
    }

    if (init_model_set(modelSet) != 0) {
        LIBSV_LOG("%s: failed on init_model_set", __func__);
        return -1;
    }

    libsv_state = LIBSV_STATE_LISTEN;
    return 0;
}

int SV::process_listen()
{
    if (libsv_state != LIBSV_STATE_LISTEN) {
        LIBSV_LOG("%s: incorrect state", __func__);
        return -1;
    }

    if ( frameFeatData.frameFeatNum == SV_LISTEN_MAX_FRAMENUM ) {
        LIBSV_LOG("%s: feat already sufficient", __func__);
        return 0;
    }

    int ret = 0;

    ret = do_feat_extr(frameFeatData, frameBuf, preprocBuf);
    if (ret == 0) {
        return 0;
    } else if (ret != 1) {
        LIBSV_LOG("%s: failed on do_feat_extr", __func__);
        return -1;
    }

    if (score_model_set(modelSet, frameFeatData) != 0) {
        LIBSV_LOG("%s: failed on score_model_set", __func__);
        return -1;
    }

    if ( frameFeatData.frameFeatNum == SV_LISTEN_MAX_FRAMENUM ) {
        double normScore = final_score_model_set(modelSet, frameFeatData);
        double selfScore = modelSet.userModel.finalScore;
        //LIBSV_LOG("%s: score %lf", __func__, score);
        if ( normScore > SV_NORMSCORE_THRD && selfScore > SV_SELFSCORE_THRD) {
            sv_result = LIBSV_CB_LISTEN_SUCCESS;
        } else {
            sv_result = LIBSV_CB_LISTEN_FAILURE;
        }
        // XXX: temporal smooth can add here if needed
    }
    return 0;
}

int SV::stop_listen()
{
    if (libsv_state != LIBSV_STATE_LISTEN) {
        LIBSV_LOG("%s: incorrect state", __func__);
        return -1;
    }

    uninit_model_set(modelSet);

    libsv_state = LIBSV_STATE_INIT;
    return 0;
}

int SV::start_enroll(const char *user)
{
    if (libsv_state != LIBSV_STATE_INIT) {
        LIBSV_LOG("%s: incorrect state", __func__);
        return -1;
    }

    sv_result = LIBSV_CB_DEF;

    if (user == NULL) {
        LIBSV_LOG("%s: invalid arg", __func__);
        return -1;
    }

    char tempbuf[256] = { 0 };
    strncpy(tempbuf, user, 255);
    enrollName = tempbuf;

    if (impstInfo.index(enrollName) >= 0) {
        impstInfo.del(enrollName);
        LIBSV_LOG("%s: user %s already exist; deleted",
            __func__, tempbuf);
    }

    inputLen = 0;
    frameFeatData.frameFeatNum = 0;

    int ret = 0;

    if (init_model_set(modelSet) != 0) {
        LIBSV_LOG("%s: failed on init_model_set", __func__);
        return -1;
    }

    // init enroll feature matrix
    enrollFeatMatrix.rows          = SV_ENROLL_MAX_FRAMENUM;
    enrollFeatMatrix.cols          = MFCC_FEATDIM;
    enrollFeatMatrix.dataType      = AUD_DATATYPE_INT32S;
    ret = createMatrix( &enrollFeatMatrix );
    LIBSV_ASSERT( ret == 0 );

    libsv_state = LIBSV_STATE_ENROLL;
    return 0;
}

int SV::process_enroll()
{
    if (libsv_state != LIBSV_STATE_ENROLL) {
        LIBSV_LOG("%s: incorrect state", __func__);
        return -1;
    }

    int  ret;

    if (frameFeatData.frameFeatNum >= SV_ENROLL_MAX_FRAMENUM ) {
        LIBSV_LOG("%s: feat already sufficient", __func__);
        return 0;
    }

    ret = do_feat_extr(frameFeatData, frameBuf, preprocBuf);
    if (ret == 0) {
        return 0;
    } else if (ret != 1) {
        LIBSV_LOG("%s: failed on do_feat_extr", __func__);
        return -1;
    }
    memcpy(enrollFeatMatrix.pInt32s +
            (frameFeatData.frameFeatNum - 1) * enrollFeatMatrix.cols,
            frameFeatData.frameFeat.featureMatrix.pInt32s,
            enrollFeatMatrix.cols * sizeof(enrollFeatMatrix.pInt32s[0]));

    if ( frameFeatData.frameFeatNum == SV_ENROLL_MAX_FRAMENUM ) {
        sv_result = LIBSV_CB_ENROLL_SUFFICIENT;
    }

    return 0;
}

int SV::generate_model(ModelSet &ms, FrameFeatData &ffd, std::string &name)
{
    int  ret = 0;
    AUD_Error error = AUD_ERROR_NONE;

    // map speaker model
    void *hAdaptedGmm = NULL;

    error = gmm_clone( &hAdaptedGmm, ms.ubmModel.hModel, (AUD_Int8s*)name.c_str() );
    LIBSV_ASSERT( error == AUD_ERROR_NONE );

    AUD_Matrix trainMatrix;
    trainMatrix.rows      = ffd.frameFeatNum;
    trainMatrix.cols      = MFCC_FEATDIM;
    trainMatrix.dataType  = AUD_DATATYPE_INT32S;
    trainMatrix.pInt32s   = enrollFeatMatrix.pInt32s;

    // adapt GMM
    error = gmm_adapt( hAdaptedGmm, &trainMatrix, abortPostEnroll );
    LIBSV_ASSERT( error == AUD_ERROR_NONE );

    if (abortPostEnroll) {
        gmm_free( &hAdaptedGmm );
        return 0;
    }

    //export feature
    std::string recFile = LIBSV_RECORDS_DIR;
    recFile += "/" + name + std::string(".feat");
    FILE *fpRec = fopen(recFile.c_str(), "wb");
    LIBSV_ASSERT( fpRec );
    int size = ffd.frameFeatNum * enrollFeatMatrix.cols;
    fwrite(enrollFeatMatrix.pInt32s, sizeof(enrollFeatMatrix.pInt32s[0]), size, fpRec);
    fclose(fpRec);
    fpRec = NULL;

    // export gmm
    std::string modelFile = LIBSV_MODELS_DIR;
    modelFile += "/" + name + ".gmm";
    FILE *fpGmm = fopen( modelFile.c_str(), "wb" );
    LIBSV_ASSERT( fpGmm );

    error = gmm_export( hAdaptedGmm, fpGmm );
    LIBSV_ASSERT( error == AUD_ERROR_NONE );

    gmm_free( &hAdaptedGmm );

    fclose( fpGmm );
    fpGmm = NULL;

    return 0;
}

int SV::compute_impst_score_from_rec(int row, int col)
{
    ModelSet ms;
    ms.userModel.id = col;
    ms.userModel.name = impstInfo.row(col).name;

    int *featBuf = frameFeatData.frameFeat.featureMatrix.pInt32s;
    int featCount = 0;

    std::string name = impstInfo.row(row).name;
    std::string path = LIBSV_RECORDS_DIR;
    path += "/" + name + ".feat";
    FILE *fp = NULL;

    if (init_model_set(ms) != 0) {
        goto compute_impst_score_from_rec_fail1;
    }

    fp = fopen(path.c_str(), "rb");
    if (fp == NULL) {
        goto compute_impst_score_from_rec_fail2;
    }

    while (!feof(fp) && featCount < SV_LISTEN_MAX_FRAMENUM) {
        //Check Point
        if (abortPostEnroll) {
            goto compute_impst_score_from_rec_abort;
        }
        fread(featBuf, sizeof(featBuf[0]), MFCC_FEATDIM, fp);
        if (score_model_set(ms, frameFeatData) != 0) {
            goto compute_impst_score_from_rec_fail3;
        }
        ++featCount;
    }
    if (featCount <= 0) {
        goto compute_impst_score_from_rec_fail3;
    }

    impstInfo.item(row, col).score = (ms.userModel.score - ms.ubmModel.score) / featCount;
compute_impst_score_from_rec_abort:
    fclose(fp);
    uninit_model_set(ms);
    return 0;
compute_impst_score_from_rec_fail3:
    fclose(fp);
compute_impst_score_from_rec_fail2:
    uninit_model_set(ms);
compute_impst_score_from_rec_fail1:
    return -1;
}

//Return: 0 for success; 1 for confusion; -1 failed
int SV::choose_imposter_by_score(int row)
{
    std::vector<int> inxVec;
    for (int i = 0; i < impstInfo.count(); ++i) {
        if (i == row)
            continue;
        inxVec.push_back(i);
    }

    for (int i = 0; i < (int)inxVec.size(); ++i) {
        int m = i;
        for (int j = i + 1; j < (int)inxVec.size(); ++j) {
            double dm = impstInfo.item(row, inxVec[m]).score;
            double dj = impstInfo.item(row, inxVec[j]).score;
            if (dj > dm) {
                m = j;
            }
        }
        int t;
        t = inxVec[i];
        inxVec[i] = inxVec[m];
        inxVec[m] = t;
    }


    for (int i = 0; i < impstInfo.count(); ++i) {
        impstInfo.item(row, i).chosen = false;
    }
    for (int i = 0; i < TNORM_IMPOSTER_NUM && i < (int)inxVec.size(); ++i) {
        impstInfo.item(row, inxVec[i]).chosen = true;
    }
    if (inxVec.size() > 0 &&
        impstInfo.item(row, row).score <=
        impstInfo.item(row, inxVec[0]).score) {
        return 1;
    }
    return 0;
}

int SV::post_enroll()
{
    int userIndex = -1;
    int confuseCount = 0;

    if (libsv_state != LIBSV_STATE_ENROLL) {
        LIBSV_LOG("%s: incorrect state", __func__);
        return -1;
    }

    // Check Point
    if (abortPostEnroll) {
        goto libsv_post_enroll_finish;
    }
    if (generate_model(modelSet, frameFeatData, enrollName) != 0) {
        LIBSV_LOG("%s: failed on generate_model", __func__);
        return -1;
    }
    // Check Point
    if (abortPostEnroll) {
        goto libsv_post_enroll_finish;
    }

    userIndex = impstInfo.add(enrollName, true);

    for (int i = 0; i < impstInfo.count(); ++i) {
        if (!impstInfo.row(i).canVerify)
            continue;
        for (int j = 0; j < impstInfo.count(); ++j) {
            // Check Point
            if (abortPostEnroll) {
                goto libsv_post_enroll_finish;
            }
            if (i == userIndex || j == userIndex) {
                LIBSV_LOG("%s: compute score %d %d", __func__, i, j);
                if (compute_impst_score_from_rec(i, j) != 0) {
                    LIBSV_LOG("%s: failed on compute_impst_score_from_rec (%d, %d)",
                        __func__, i, j);
                    return -1;
                }
            }
        }
    }
    // Check Point
    if (abortPostEnroll) {
        goto libsv_post_enroll_finish;
    }

    confuseCount = 0;
    for (int i = 0; i < impstInfo.count(); ++i) {
        // Check Point
        if (abortPostEnroll) {
            goto libsv_post_enroll_finish;
        }
        if (!impstInfo.row(i).canVerify)
            continue;
        confuseCount += choose_imposter_by_score(i);
    }

libsv_post_enroll_finish:
    if (abortPostEnroll) {
        sv_result = LIBSV_CB_ENROLL_ABORTED;
        return 0;
    }
    if (confuseCount == 0) {
        sv_result = LIBSV_CB_ENROLL_SUCCESS;
    } else {
        sv_result = LIBSV_CB_ENROLL_CONFUSED;
    }
    return 0;
}

int SV::abort_post_enroll()
{
    if (libsv_state != LIBSV_STATE_ENROLL) {
        LIBSV_LOG("%s: incorrect state", __func__);
        return -1;
    }
    abortPostEnroll = true;
    LIBSV_LOG("%s: aborting post enroll", __func__);
    return 0;
}

int SV::confirm_enroll()
{
    if (libsv_state != LIBSV_STATE_ENROLL) {
        LIBSV_LOG("%s: incorrect state", __func__);
        return -1;
    }

    if (abortPostEnroll) {
        LIBSV_LOG("%s: cannot confirm enroll during aborting", __func__);
        return -1;
    }

    destroyMatrix( &enrollFeatMatrix );

    uninit_model_set(modelSet);

    if (!impstInfo.save(LIBSV_IMPSTINFO_PATH)) {
        LIBSV_LOG("%s: failed on impstInfo.save", __func__);
        return -1;
    }
    libsv_state = LIBSV_STATE_INIT;
    return 0;
}

int SV::cancel_enroll()
{
    if (libsv_state != LIBSV_STATE_ENROLL) {
        LIBSV_LOG("%s: incorrect state", __func__);
        return -1;
    }

    abortPostEnroll = false;

    destroyMatrix( &enrollFeatMatrix );

    uninit_model_set(modelSet);

    if (!impstInfo.load(LIBSV_IMPSTINFO_PATH)) {
        impstInfo.clear();
        impstInfo.save(LIBSV_IMPSTINFO_PATH);
        LIBSV_LOG("%s: failed on impstInfo.load", __func__);
        return -1;
    }

    libsv_state = LIBSV_STATE_INIT;
    return 0;
}

int SV::push_data(short *buf, int len)
{
    if (libsv_state != LIBSV_STATE_LISTEN &&
        libsv_state != LIBSV_STATE_ENROLL) {
        LIBSV_LOG("%s: incorrect state", __func__);
        return -1;
    }
    if (len != FRAME_STRIDE) {
        LIBSV_LOG( "Pushed length %d is not equal to FRAME_STRIDE %d", len, FRAME_STRIDE );
        return -1;
    }
#ifdef RECORD_PCM
    if (fp_pcm != NULL)
        fwrite(buf, len, sizeof(buf[0]), fp_pcm);
#endif
    if (inputLen == FRAME_LEN) {
        memcpy(frameBuf, frameBuf + FRAME_STRIDE, FRAME_STRIDE * BYTES_PER_SAMPLE);
        inputLen = FRAME_STRIDE;
    }
    memcpy(frameBuf + inputLen, buf, BYTES_PER_SAMPLE * FRAME_STRIDE);
    inputLen += FRAME_STRIDE;
    return inputLen == FRAME_LEN ? 1 : 0;
}

int SV::get_user_list(char (*names)[LIBSV_USER_NAME_LEN], int *num)
{
    if (num == NULL) {
        LIBSV_LOG("%s: invalid argument", __func__);
        return -1;
    }
    if (names == NULL) {
        *num = impstInfo.count();
        return 0;
    }
    int i;
    for ( i = 0; i < impstInfo.count() && i < *num; ++i ) {
        const char * name = impstInfo.row(i).name.c_str();
        strncpy(names[i], name, LIBSV_USER_NAME_LEN);
        names[i][LIBSV_USER_NAME_LEN - 1] = 0;
    }
    *num = i;
    return 0;
}

int SV::remove_user(const char *user)
{
    if (libsv_state != LIBSV_STATE_NONE &&
        libsv_state != LIBSV_STATE_INIT) {
        LIBSV_LOG("%s: incorrect state", __func__);
        return -1;
    }

    if (user == NULL) {
        LIBSV_LOG("%s: invalid argument", __func__);
        return -1;
    }

    char tempbuf[256] = { 0 };
    strncpy(tempbuf, user, 255);

    if (impstInfo.del(tempbuf) < 0) {
        LIBSV_LOG("%s: user %s not exist", __func__, tempbuf);
        return -1;
    }
    for (int i = 0; i < impstInfo.count(); ++i) {
        if (!impstInfo.row(i).canVerify)
            continue;
        choose_imposter_by_score(i);
    }
    if (!impstInfo.save(LIBSV_IMPSTINFO_PATH)) {
        LIBSV_LOG("%s: failed on impstInfo.save", __func__);
        return -1;
    }
    std::string path;
    path = LIBSV_MODELS_DIR + std::string("/") + user + ".gmm";
    unlink(path.c_str());
    path = LIBSV_RECORDS_DIR + std::string("/") + user + ".feat";
    unlink(path.c_str());
    return 0;
}

int SV::get_state()
{
    return libsv_state;
}

int SV::get_frm_num()
{
    return frameFeatData.frameFeatNum;
}

int SV::get_result()
{
    return sv_result;
}

