# -*- coding:utf-8 -*-
#! /usr/bin/python

from mysql_utils import connectDb, disconnectDb
# import datetime
import math

def get_hindex(m):
    s = [0] * (len(m) + 1)
    for i in xrange(len(m)):
        idx = min(len(m), m[i])
        s[idx] += 1
    # print s, len(s)
    sum_ = 0
    h = 0
    for i in xrange(len(s) - 1, -1, -1):
        sum_ += s[i]
        if sum_ <= i and h < sum_:
            h = sum_
    return h

def is_chinese(uchar):
    if uchar >= u'\u4e00' and uchar <= u'\u9fa5':
        return True
    else:
        return False

def fill_text_to_print_width(text, width):
    # stext = str(text)
    # utext = text.decode("utf-8")
    cn_count = 0
    # print len(text)
    for u in text:
        if is_chinese(u):
            cn_count += 1
    # print cn_count
    return text + " " * (width - cn_count - len(text))

if __name__ == "__main__":

    db_para = {'host':'localhost', \
               'user':'root',\
               'pwd':'matrix@88',\
               'db':'weibo',\
               'port':3306,\
               'charset':'utf8'}
    db = connectDb(db_para)

    db_cur = db.cursor()

    sql = 'select uid from user_info'
    cnt = db_cur.execute(sql)
    users = db_cur.fetchall()
    # print users

    user_kpi = {}
    user_info = {}
    # get each user's influence metric
    for user in users:
        print user
        sql = 'select name from user_info where uid=%s' % (user)
        db_cur.execute(sql)
        name = db_cur.fetchall()[0]
        # print name

        sql = 'select num_fans, num_follows, num_posts, last_post_time, post100time from user_status where uid=%s' % (user)
        db_cur.execute(sql)
        status = db_cur.fetchall()
        # print len(status[0])
        # 1. Weibo Creation Rate: #weibo per day
        last_post_time = status[0][3]
        post100time = status[0][4]
        # print last_post_time, post100time
        day_interval = (last_post_time - post100time).days
        # print day_interval
        WCR = 100.0 / day_interval
        # print WCR

        # 2. OOM: Magnitude of his/her followers: log10(#follower)
        num_fans = status[0][0]
        OOM = math.log10(num_fans)
        # print OOM

        # 3. fans coverage: log10(#followers / #following + 1)
        num_follows = status[0][1]
        coverage = math.log10(num_fans / float(num_follows) + 1.0)
        # print coverage

        # 4. Influence Metrics
        IM = WCR * OOM * coverage
        # print IM

        # 4. broadcasting cap: h-index for repost and vote
        sql = 'select num_repost from user_post where uid=%s' % (user)
        db_cur.execute(sql)
        posts = db_cur.fetchall()
        # print posts

        num_repost = []
        for post in posts:
            num_repost.append(post[0])
        # print num_repost

        repost_hindex = get_hindex(num_repost)
        # print repost_hindex

        sql = 'select num_vote from user_post where uid=%s' % (user)
        db_cur.execute(sql)
        votes = db_cur.fetchall()
        # print votes

        num_vote = []
        for vote in votes:
            num_vote.append(vote[0])
        # print num_vote

        vote_hindex = get_hindex(num_vote)
        # print vote_hindex

        user_kpi[user] = [WCR, OOM, coverage, IM, repost_hindex, vote_hindex]
        user_info[user] = name
    # print kpi_user
    print "User Influences:"
    print "------------------------------------------"
    print "  %-12s  %-14s  %-20s  %-20s  %-22s  %-16s  %-16s  %-16s" % ("User", "Name", "Weibo Creation Rate", "Order Of Magnitude", "Log Fans Over Follows", "Influence Metric", "repost h-index", "vote h-index")
    for user in user_kpi:
        print "  %-12s  %-s  %-20s  %-20s  %-22s  %-16s  %-16s  %-16s" % \
              (user[0], fill_text_to_print_width(user_info[user][0], 14), user_kpi[user][0],  user_kpi[user][1], user_kpi[user][2], user_kpi[user][3], user_kpi[user][4], user_kpi[user][5])
    print "------------------------------------------"
    disconnectDb(db)
    print "done"
