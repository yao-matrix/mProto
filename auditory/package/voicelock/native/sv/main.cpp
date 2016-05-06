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

#include <utils/Errors.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "io/file/io.h"
#include "SpeakerVerification.h"
#include "ImposterInfo.h"
#include "AudioConfig.h"

using namespace android;
typedef struct timeval TimeVal;

SV sv;

int list_users()
{
    int n = 0;
    sv.init();
    sv.get_user_list(NULL, &n);
    if (n == 0) {
        printf("No User\n");
        return 0;
    }
    char (*names)[LIBSV_USER_NAME_LEN];
    names = (char (*)[LIBSV_USER_NAME_LEN])calloc(n, sizeof(names[0]));
    sv.get_user_list(names, &n);
    printf("Users:\n");
    for (int i = 0; i < n; ++i) {
        printf("\t%s\n", names[i]);
    }
    free(names);
    sv.uninit();
    return 0;
}

int remove(const char *user)
{
    int ret;
    sv.init();
    ret = sv.remove_user(user);
    if (ret != 0) {
        printf("Failed to remove\n");
    }
    sv.uninit();
    return ret;
}

int listen(const char *user, const char *test_file)
{
    AUD_Summary wavHead;
    if (parseWavFromFile((AUD_Int8s*)test_file, &wavHead) != 0) {
        printf("Failed : parseWavFromFile\n");
        return -1;
    }
    int size = wavHead.dataChunkBytes;
    short *buf = (short*)calloc(1, size);
    int inx = 0;
    int samples = readWavFromFile((AUD_Int8s*)test_file, buf, size);
    printf("samples %d, size %d\n", samples, size);
    sv.init();
    if (sv.start_listen(user) != 0) {
        printf("Failed : sv.start_listen\n");
        return -1;
    }
    //FILE *fp = fopen("/sdcard/test.pcm", "wb");
    TimeVal t0, t1, td;
    gettimeofday(&t0, NULL);
    while (samples > FRAME_STRIDE) {
        int res = sv.push_data(buf + inx, FRAME_STRIDE);
        //fwrite(buf + inx, FRAME_STRIDE * 2, 1, fp);
        inx += FRAME_STRIDE;
        samples -= FRAME_STRIDE;
        if (res == 1) {
            sv.process_listen();
        }
        if (sv.get_result() != LIBSV_CB_DEF)
            break;
    }
    gettimeofday(&t1, NULL);
    timersub(&t1, &t0, &td);
    long long diff = td.tv_sec * 1000000 + td.tv_usec;
    printf("cost %lld usec\n", diff);
    //fclose(fp);
    sv.stop_listen();
    sv.uninit();
    free(buf);
    if (sv.get_result() == LIBSV_CB_LISTEN_SUCCESS) {
        printf("Listen Success\n");
    } else if (sv.get_result() == LIBSV_CB_LISTEN_FAILURE) {
        printf("Listen Failure\n");
    }
    return 0;
}

int enroll(const char *user, const char *test_file)
{
    AUD_Summary wavHead;
    if (parseWavFromFile((AUD_Int8s*)test_file, &wavHead) != 0) {
        printf("Failed : parseWavFromFile\n");
        return -1;
    }
    int size = wavHead.dataChunkBytes;
    short *buf = (short*)calloc(1, size);
    int inx = 0;
    int samples = readWavFromFile((AUD_Int8s*)test_file, buf, size);
    printf("samples %d, size %d\n", samples, size);
    sv.init();
    if (sv.start_enroll(user) != 0) {
        printf("Failed : sv.start_enroll\n");
        goto enroll_exit;
    }
    while (samples > FRAME_STRIDE) {
        int res = sv.push_data(buf + inx, FRAME_STRIDE);
        inx += FRAME_STRIDE;
        samples -= FRAME_STRIDE;
        if (res == 1) {
            if (sv.process_enroll() != 0) {
                printf("Failed : sv.process_enroll\n");
                goto enroll_exit;
            }
        }
        if (sv.get_result())
            break;
    }
    sv.post_enroll();
    if (sv.get_result() == LIBSV_CB_ENROLL_CONFUSED) {
        while (true) {
            printf("New model is confusing!\ny for conferm; n for cancel:");
            char answer;
            scanf("%c\n", &answer);
            if (answer == 'y') {
                sv.confirm_enroll();
                break;
            } else if (answer == 'n') {
                sv.cancel_enroll();
                break;
            }
        }
    } else if (sv.get_result() == LIBSV_CB_ENROLL_SUCCESS) {
        sv.confirm_enroll();
    }
enroll_exit:
    if (sv.get_result() == LIBSV_CB_ENROLL_SUCCESS) {
        printf("Enroll Success\n");
    } else if (sv.get_result() == LIBSV_CB_ENROLL_FAIL) {
        printf("Enroll Failure\n");
    } else if (sv.get_result() == LIBSV_CB_ENROLL_CONFUSED) {
        printf("Enroll Confused\n");
    } else if (sv.get_result() == LIBSV_CB_ENROLL_SUFFICIENT) {
        printf("Enroll Failure : Error in Post Enroll\n");
    }
    sv.uninit();
    free(buf);
    return 0;
}

void devtest()
{
    ImposterInfo ii, jj;
    ii.add("aaa", true);
    ii.item(0,0).score = 1.234f;
    ii.item(0,0).chosen = 1;
    ii.add("bbb", false);
    ii.item(1,1).score = 2.345f;
    ii.item(1,1).chosen = 1;
    ii.add("ccc", true);
    ii.item(2,2).score = 3.456f;
    ii.item(2,2).chosen = 1;
    ii.save("/sdcard/testii.txt");
    jj.load("/sdcard/testii.txt");
    jj.save("/sdcard/testii2.txt");
}

void usage(int argc, char *argv[])
{
    printf("Usage: %s list | remove <user> | "
        "listen <user> <file> | enroll <user> <file> | "
        "devtest\n", argv[0]);
}

int main( int argc, char *argv[] )
{
    if (argc < 2) {
        usage(argc, argv);
        return 0;
    }
    if (strcmp(argv[1], "list") == 0) {
        list_users();
    } else if (strcmp(argv[1], "remove") == 0) {
        if (argc < 3) {
            usage(argc, argv);
            return 0;
        }
        remove(argv[2]);
    } else if (strcmp(argv[1], "listen") == 0) {
        if (argc < 4) {
            usage(argc, argv);
            return 0;
        }
        listen(argv[2], argv[3]);
    } else if (strcmp(argv[1], "enroll") == 0) {
        if (argc < 4) {
            usage(argc, argv);
            return 0;
        }
        enroll(argv[2], argv[3]);
    } else if (strcmp(argv[1], "devtest") == 0) {
        devtest();
    } else {
        usage(argc, argv);
        return 0;
    }
    return 0;
}
