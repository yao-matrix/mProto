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

#if !defined( _SPEAKERVERIFICATION_H )
#define _SPEAKERVERIFICATION_H

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"
#include "AudioConfig.h"
#include "ImposterInfo.h"
#include <stdio.h>
#include <vector>
#include <string>

enum
{
    LIBSV_CB_DEF = 0,
    LIBSV_CB_LISTEN_SUCCESS,
    LIBSV_CB_LISTEN_FAILURE,
    LIBSV_CB_ENROLL_SUCCESS,
    LIBSV_CB_ENROLL_FAIL,
    LIBSV_CB_ENROLL_SUFFICIENT,
    LIBSV_CB_ENROLL_CONFUSED,
    LIBSV_CB_ENROLL_ABORTED,
};

#define LIBSV_USER_NAME_LEN 64

#define LIBSV_ROOT_DIR "/sdcard"
#define LIBSV_SV_DIR "/sdcard/sv"
#define LIBSV_RECORDS_DIR "/sdcard/sv/records"
#define LIBSV_MODELS_DIR "/sdcard/sv/models"
#define LIBSV_UBMMODEL_PATH  "/sdcard/sv/ubm-512.gmm"
#define LIBSV_IMPSTINFO_PATH   "/sdcard/sv/impstinfo.csv"
#define LIBSV_IMAGE_ROOT_DIR "system/vendor/intel/apps/aware/voicelock"

typedef enum {
    LIBSV_STATE_NONE = 0,
    LIBSV_STATE_INIT,
    LIBSV_STATE_LISTEN,
    LIBSV_STATE_ENROLL,
} libsv_state_t;

struct FrameFeatData {
    FrameFeatData() {
        hVadHandle = NULL;
        hFeatHandle = NULL;
        featCreated = false;
        frameFeatNum = 0;
    }
    void *hVadHandle;
    void *hFeatHandle;
    bool featCreated;
    AUD_Feature frameFeat;
    int frameFeatNum;
};

struct ModelInfo {
    ModelInfo() {
        id = -1;
        score = 0.0;
        finalScore = 0.0;
        hModel = NULL;
    }
    int id;
    std::string name;
    double score;
    double finalScore;
    void *hModel;
};

typedef std::vector<ModelInfo> ModelVec;

struct ModelSet
{
    ModelInfo ubmModel;
    ModelInfo userModel;
    ModelVec modelVec;
};

class SV
{
public:
    SV();
    ~SV();
    int init();
    int uninit();
    int start_listen(const char *user);
    int process_listen();
    int stop_listen();
    int start_enroll(const char *user);
    int process_enroll();
    int post_enroll();
    int abort_post_enroll();
    int confirm_enroll();
    int cancel_enroll();
    int push_data(short *buf, int len);
    int get_user_list(char (*names)[LIBSV_USER_NAME_LEN], int *num);
    int remove_user(const char *user);
    int get_state();
    int get_frm_num();
    int get_result();
private:
    int init_feat_extr(FrameFeatData &ffd);
    int uninit_feat_extr(FrameFeatData &ffd);
    int do_feat_extr(FrameFeatData &ffd, short *fbuf, short *pbuf);
    void* load_model(const std::string &path);
    int init_model_set(ModelSet &ms);
    int uninit_model_set(ModelSet &ms);
    double calc_aggr_score(void *hModel, AUD_Matrix *featMat,
        AUD_Vector *compLlr, AUD_Vector *bestIndex, bool sortComp);
    int score_model_set(ModelSet &ms, FrameFeatData &ffd);
    double final_score_model_set(ModelSet &ms, FrameFeatData &ffd);
    int check_filesys();
    int copy_file(const std::string &dst, const std::string &src);
    int copy_dir(const std::string &dst, const std::string &src);
    int generate_model(ModelSet &ms, FrameFeatData &ffd, std::string &name);
    int compute_impst_score_from_rec(int row, int col);
    int choose_imposter_by_score(int row);
private:
    int sv_result;
    libsv_state_t libsv_state;
    short frameBuf[FRAME_LEN];
    short preprocBuf[FRAME_LEN];
    int inputLen;
    AUD_Matrix enrollFeatMatrix;
    ImposterInfo impstInfo;
    std::string enrollName;
    bool abortPostEnroll;
    FrameFeatData frameFeatData;
    ModelSet modelSet;
#ifdef RECORD_PCM
    FILE* fp_pcm;
#endif
};

#endif
