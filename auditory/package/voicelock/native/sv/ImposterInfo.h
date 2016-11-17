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
