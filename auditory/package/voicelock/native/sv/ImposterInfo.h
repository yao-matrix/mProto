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

#if !defined( _IMPOSTERINFO_H )
#define _IMPOSTERINFO_H

#include "AudioConfig.h"
#include <vector>
#include <string>

class ImposterInfo
{
public:
    struct Item
    {
        Item();
        float score;
        int chosen;
    };
    struct Row
    {
        Row();
        std::string name;
        bool canVerify;
    };
    ImposterInfo();
    ~ImposterInfo();
    bool save(const std::string &path);
    bool load(const std::string &path);
    int add(const std::string &name, bool canVerify);
    int del(const std::string &name);
    void clear();
    int count();
    Item& item(int row, int col);
    Row& row(int n);
    Item& emptyItem();
    Row& emptyRow();
    int index(const std::string &name);
private:
    unsigned mCount;
    typedef std::vector<Row> row_vec;
    typedef std::vector<Item> item_row;
    typedef std::vector<item_row> item_tab;
    row_vec mRows;
    item_tab mTable;
    Row mEmptyRow;
    Item mEmptyItem;
};



#endif
