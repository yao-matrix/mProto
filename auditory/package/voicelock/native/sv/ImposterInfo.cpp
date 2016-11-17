#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <ctype.h>
#include <cutils/properties.h>

#include <sys/types.h>
#include <dirent.h>
#include <fstream>
#include <sstream>

#include "ImposterInfo.h"


ImposterInfo::Item::Item()
{
    score = 0.0f;
    chosen = 0;
}

ImposterInfo::Row::Row()
{
    canVerify = false;
}

ImposterInfo::ImposterInfo()
{
    mCount = 0;
}

ImposterInfo::~ImposterInfo()
{
}

typedef std::vector<std::string> str_vec;
typedef std::vector<bool> bool_vec;

static void splitStr(std::string &str, char c, str_vec &split)
{
    split.clear();
    size_t p = 0, q = 0;
    while ( ( p = str.find(c, q) ) != std::string::npos ) {
        split.push_back( str.substr(q, p - q) );
        q = p + 1;
    }
    if ( q < str.length() ) {
        split.push_back( str.substr(q, str.length() - q) );
    }
}

static void joinStr(std::string &str, char c, str_vec &split)
{
    str = "";
    if (split.size() == 0)
        return;
    unsigned i;
    for (i = 0; i < split.size() - 1; ++i) {
        str += split[i];
        str += c;
    }
    str += split[i];
}


template<typename S, typename D>
static void projVec(std::vector<D> &dst, std::vector<S> &src, void (*proj)(D &d, S &s))
{
    for (unsigned i = 0; i < src.size(); ++i) {
        proj(dst[i], src[i]);
    }
}

static void row_name(std::string &n, ImposterInfo::Row &r)
{
    n = r.name;
}

static void row_cv(std::string &c, ImposterInfo::Row &r)
{
    c = r.canVerify ? "Y" : "N";
}

static void name_row(ImposterInfo::Row &r, std::string &n)
{
    r.name = n;
}

static void cv_row(ImposterInfo::Row &r, std::string &c)
{
    r.canVerify = c == "Y" ? true : false;
}

bool ImposterInfo::save(const std::string &path)
{
    std::string s1, s2;
    str_vec strs1, strs2;
    std::fstream fs(path.c_str(), std::fstream::out);

    if (fs.fail()) {
        return false;
    }

    strs1.resize(mRows.size());
    projVec(strs1, mRows, row_name);
    joinStr(s1, ',', strs1);
    fs << s1 << std::endl;
    projVec(strs1, mRows, row_cv);
    joinStr(s1, ',', strs1);
    fs << s1 << std::endl;

    for (unsigned i = 0; i < mCount; ++i) {
        strs1.clear();
        if (mRows[i].canVerify) {
            for (unsigned j = 0; j < mCount; ++j) {
                Item &ent = item(i, j);
                std::stringstream ss;
                ss << ent.score << '/' << ent.chosen;
                ss >> s2;
                strs1.push_back(s2);
            }
        } else {
            strs1.assign(mCount, "X");
        }
        joinStr(s1, ',', strs1);
        fs << s1 << std::endl;
    }

    fs.close();
    return true;
}

bool ImposterInfo::load(const std::string &path)
{
    std::string s1, s2;
    str_vec strs1, strs2;
    std::fstream fs(path.c_str(), std::fstream::in);

    if (fs.fail()) {
        return false;
    }

    fs >> s1;
    fs >> s2;
    if (fs.fail()) {
        fs.close();
        return false;
    }
    splitStr(s1, ',', strs1);
    splitStr(s2, ',', strs2);
    if (strs1.size() != strs2.size()) {
        fs.close();
        return false;
    }
    mRows.resize(strs1.size());
    projVec(mRows, strs1, name_row);
    projVec(mRows, strs2, cv_row);
    mCount = mRows.size();

    mTable.clear();
    mTable.resize(mCount);
    for (unsigned i = 0; i < mCount; ++i) {
        mTable[i].resize(mCount);
    }

    for (unsigned i = 0; i < mCount; ++i) {
        fs >> s1;
        if (fs.fail()) {
            fs.close();
            return false;
        }
        splitStr(s1, ',', strs1);
        if (strs1.size() != mCount) {
            fs.close();
            return false;
        }
        if (!mRows[i].canVerify) {
            continue;
        }
        for (unsigned j = 0; j < mCount; ++j) {
            Item &ent = item(i, j);
            splitStr(strs1[j], '/', strs2);
            if (strs2.size() != 2) {
                fs.close();
                return false;
            }
            std::stringstream ss0, ss1;
            ss0 << strs2[0];
            ss0 >> ent.score;
            ss1 << strs2[1];
            ss1 >> ent.chosen;
            if (ss0.fail() || ss1.fail()) {
                fs.close();
                return false;
            }
        }
    }

    fs.close();
    return true;
}

int ImposterInfo::add(const std::string &name, bool canVerify)
{
    mCount++;
    Row nr;
    nr.name = name;
    nr.canVerify = canVerify;
    mRows.push_back(nr);
    mTable.resize(mCount);
    for (unsigned i = 0; i < mCount; ++i) {
        mTable[i].resize(mCount);
    }
    return mCount-1;
}

int ImposterInfo::del(const std::string &name)
{
    int index = this->index(name);
    if (index == -1)
        return -1;
    --mCount;
    mRows.erase(mRows.begin() + index);
    mTable.erase(mTable.begin() + index);
    for (unsigned i = 0; i < mCount; ++i) {
        mTable[i].erase(mTable[i].begin() + index);
    }
    return index;
}

void ImposterInfo::clear()
{
    mCount = 0;
    mRows.clear();
    mTable.clear();
}

int ImposterInfo::count()
{
    return mCount;
}

ImposterInfo::Item& ImposterInfo::item(int row, int col)
{
    if (row < 0 || (unsigned)row >= mCount ||
        col < 0 || (unsigned)col >= mCount)
        return mEmptyItem;
    return mTable[row][col];
}

ImposterInfo::Row& ImposterInfo::row(int n)
{
    if (n < 0 || (unsigned)n >= mCount)
        return mEmptyRow;
    return mRows[n];
}

ImposterInfo::Item& ImposterInfo::emptyItem()
{
    return mEmptyItem;
}

ImposterInfo::Row& ImposterInfo::emptyRow()
{
    return mEmptyRow;
}

int ImposterInfo::index(const std::string &name)
{
    for (unsigned i = 0; i < mCount; ++i) {
        if (mRows[i].name == name) {
            return i;
        }
    }
    return -1;
}


