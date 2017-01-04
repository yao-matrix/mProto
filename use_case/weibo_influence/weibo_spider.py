# -*- coding:utf-8 -*-
#! /usr/bin/python

import urllib2
import urllib
from bs4 import BeautifulSoup
import cookielib
import re
import datetime
import time

import lxml.html as HTML
from mysql_utils import connectDb, disconnectDb

domain = "http://weibo.cn/"

class Fetcher(object):
    def __init__(self, username = None, pwd = None, cookie_filename = None):
        self.cj = cookielib.LWPCookieJar()
        if cookie_filename is not None:
            self.cj.load(cookie_filename)
        self.cookie_processor = urllib2.HTTPCookieProcessor(self.cj)
        self.opener = urllib2.build_opener(self.cookie_processor, urllib2.HTTPHandler)
        urllib2.install_opener(self.opener)
         
        self.username = username
        self.pwd = pwd
        self.headers = {'User-Agent': 'Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:39.0) Gecko/20100101 Firefox/39.0',
                        'Connection': 'keep-alive',
                        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8'}
     
    def get_rand(self, url):
        headers = {'User-Agent': 'Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:39.0) Gecko/20100101 Firefox/39.0',
                   'Referer': ''}
        req = urllib2.Request(url, urllib.urlencode({}), headers)
        resp = urllib2.urlopen(req)
        login_page = resp.read()
        rand = HTML.fromstring(login_page).xpath("//form/@action")[0]
        passwd = HTML.fromstring(login_page).xpath("//input[@type='password']/@name")[0]
        vk = HTML.fromstring(login_page).xpath("//input[@name='vk']/@value")[0]
        # print rand, passwd, vk
        return rand, passwd, vk
     
    def login(self, username = None, pwd = None, cookie_filename = None):
        if self.username is None or self.pwd is None:
            self.username = username
            self.pwd = pwd
        assert self.username is not None and self.pwd is not None
         
        url = 'http://login.weibo.cn/login/?ns=1&revalid=2&backURL=http%3A%2F%2Fweibo.cn%2F&backTitle=%CE%A2%B2%A9&vt='
        rand, passwd, vk = self.get_rand(url)

        data = urllib.urlencode({'mobile': self.username,
                                 passwd: self.pwd,
                                 'remember': 'on',
                                 'backURL': 'http://weibo.cn/',
                                 'backTitle': '微博',
                                 'tryCount': '',
                                 'vk': vk,
                                 'submit': '登录'})
        url = 'http://login.weibo.cn/login/' + rand
        req = urllib2.Request(url, data, self.headers)
        resp = urllib2.urlopen(req)
        page = resp.read()
        link = HTML.fromstring(page).xpath("//a/@href")[0]
        # print link
        if not link.startswith('http://'):
            link = 'http://weibo.cn/%s' % link
        
        req = urllib2.Request(link, headers = self.headers)
        urllib2.urlopen(req)
        if cookie_filename is not None:
            self.cj.save(filename = cookie_filename)
        elif self.cj.filename is not None:
            self.cj.save()

        # print 'login success!'
         
    def fetch(self, url):
        # print 'fetch url: ', url
        req = urllib2.Request(url, headers = self.headers)
        return urllib2.urlopen(req).read()

def parse_post_time(post_time):
    res = None
    if post_time.find(u'月') >= 0:
        tmp1 = post_time.split(u' ')
        date = tmp1[0]. split(u'月')
        month = date[0]
        day = date[1].split(u'日')[0]
        year = datetime.date.today().year
        res = "%s-%s-%s %s:00" % (year, month, day, tmp1[1])
    elif post_time.find(u'今天') >= 0:
        tmp1 = post_time.split(u' ')
        year = datetime.date.today().year
        month = datetime.date.today().month
        day = datetime.date.today().day
        res = "%s-%s-%s %s:00" % (year, month, day, tmp1[1])
    else:
        res = post_time
    return res


def get_userinfo(fetcher, uid, db)
    global domain

    db_cur = db.cursor()

    # user info
    info_page = domain + str(uid) + "/info"
    html = fetcher.fetch(info_page)
    page = BeautifulSoup(html)
    name = page.find(text = re.compile(u'昵称:'))
    name = name.split(u':')[1]
    # print name
    gender = page.find(text = re.compile(u'性别:'))
    gender = gender.split(u':')[1]
    # print gender
    if gender == u'女':
        gender = 0
    else:
        gender = 1
    locale = page.find(text = re.compile(u'地区:'))
    locale = locale.split(u':')[1]
    # print locale
    birthday = page.find(text = re.compile(u'生日:'))
    if birthday is None:
        birthday = '0000-00-00'
    else:
        birthday = birthday.split(u':')[1]
        if len(birthday.split('-')) < 3:
            birthday = '0000-00-00'
    # print birthday

    tags = []
    buff = page.find(text = re.compile(u'达人:'))
    if buff is not None:
        tags = buff.strip().split(u':')[1].split(u' ')

    auth = page.find(text = re.compile(u'认证:'))
    if auth is not None:
        tmp = auth.strip().split(u':')[1].split(u'，')
        if len(tmp) == 1:
           tmp = tmp[0].split(u' ') 
        for item in tmp:
            if item not in tags:
                if sum([1 for m in tags if ( (item not in m) and (m not in item) ) ]) == len(tags):
                    tags.append(item)

    tag_page = page.find_all(name = 'a', attrs = {'href': re.compile('/account/privacy/tags')})
    tag_page = tag_page[0]['href']
    # print tag_page
    html = fetcher.fetch(domain + tag_page)
    page = BeautifulSoup(html)
    m = page.find_all(name = 'a', attrs = {'href': re.compile('/search/\?keyword=')})
    tmp = [x.get_text() for x in m]
    for item in tmp:
        if item not in tags:
            if sum([1 for m in tags if ( (item not in m) and (m not in item) ) ]) == len(tags):
                tags.append(item)

    tags_str = ','.join(tags)
    # print tags_str
    # for item in m:
    #    print item.get_text()

    sql = 'insert into user_info values(%d, \'%s\', %d, \'%s\', \'%s\', \'%s\')' % (uid, name, gender, locale, tags_str, birthday)
    db_cur.execute(sql)
    db.commit()


def get_user(fetcher, uid, db):
    global domain

    get_userinfo(fetcher, uid, db)

    db_cur = db.cursor()

    # get user social information(user_status)
    user_page = domain + "u/" + str(uid) 
    html = fetcher.fetch(user_page)
    page = BeautifulSoup(html)

    fans = page.find(text = re.compile(u'粉丝\['))
    fans_num = int(filter(lambda x:x.isdigit(), fans))
    # print fans_num
    follow = page.find(text = re.compile(u'关注\['))
    follow_num = int(filter(lambda x:x.isdigit(), follow))
    # print follow_num
    article = page.find(text = re.compile(u'微博\['))
    weibo_num = int(filter(lambda x:x.isdigit(), article))
    # print weibo_num

    # weibos user posted
    num_page = int(page.find(name = 'input', attrs = {'name': 'mp'})['value'])
    # print "page number: ", num_page
    user_posts = dict()
    latest = None
    post100time = None
    msg_num = 0
    for i in xrange(num_page):
        url = user_page + '?page=' + str(i + 1)
        page = fetcher.fetch(url)
        page = BeautifulSoup(page)
        msgs = page.find_all(name = 'div', attrs = {'id':re.compile('M_')})
        # print "length of msgs: ", len(msgs)
        for msg in msgs:
            msg_num += 1
            mid = msg['id']
            divs = msg.find_all(name = 'div')
            # divs[0] is content
            if latest == None:
                context = divs[-1].find(name = 'span', attrs = {'class': 'ct'})
                context = context.get_text().split(u'\xa0来自')
                post_time = context[0]
                post_time = parse_post_time(post_time)
                latest = post_time

            topics = divs[0].find_all(name = 'a', attrs = {'href': re.compile('http://weibo.cn/pages/100808topic')})
            if len(topics) > 0:
                attitude = divs[-1].find(name = 'a', attrs = {'href': re.compile('http://weibo.cn/attitude')})
                attitude = attitude.get_text()
                attitude_num = int(filter(lambda x:x.isdigit(), attitude))
                repost = divs[-1].find(name = 'a', attrs = {'href': re.compile('http://weibo.cn/repost')})
                repost = repost.get_text()
                repost_num = int(filter(lambda x:x.isdigit(), repost))
                comment = divs[-1].find(name = 'a', attrs = {'href': re.compile('http://weibo.cn/comment')})
                comment = comment.get_text()
                comment_num = int(filter(lambda x:x.isdigit(), comment))

                context = divs[-1].find(name = 'span', attrs = {'class': 'ct'})
                context = context.get_text().split(u'\xa0来自')
                # print context
                post_time = context[0]
                post_device = context[1]
                user_posts[mid] = {}
                # print post_time
                t = parse_post_time(post_time)

                user_posts[mid]['attitude'] = attitude_num
                user_posts[mid]['repost'] = repost_num
                user_posts[mid]['comment'] = comment_num
                user_posts[mid]['time'] = t
                user_posts[mid]['device'] = post_device
                # print t

            # print msg_num
            if msg_num == 100:
                context = divs[-1].find(name = 'span', attrs = {'class': 'ct'})
                context = context.get_text().split(u'\xa0来自')
                post_time = context[0]
                post100time = parse_post_time(post_time)
                # print post100time
        time.sleep(1)

    # print user_posts

    for key in user_posts:  
        sql = 'insert into user_post values(%d, \'%s\', \'%s\', \'N/A\', %d, %d, %d, \'%s\')' % (uid, key, user_posts[key]['time'], user_posts[key]['repost'], user_posts[key]['attitude'], \
                                                                                             user_posts[key]['comment'], user_posts[key]['device'])
        db_cur.execute(sql)
        db.commit()

    sql = 'insert into user_status values(%d, %d, %d, %d, \'%s\', \'%s\')' % (uid, fans_num, follow_num, weibo_num, latest, post100time)
    db_cur.execute(sql)
    db.commit()



    # list out fans
    fans_page = domain + str(uid) + "/fans"
    # print fans
    html = fetcher.fetch(fans_page)
    page = BeautifulSoup(html)
    num_page = int(page.find(name = 'input', attrs = {'name':'mp'})['value'])
    # print num_page

    fans_list = []
    for i in xrange(num_page):
        url = fans_page + '?vt=4&page=' + str(i + 1)
        page = fetcher.fetch(url)
        page = BeautifulSoup(page)
        m = page.find_all(name = 'a', attrs = {'href': re.compile('http://weibo.cn/u/')})
        for j in xrange(len(m)):
            u = m[j]['href']
            l = u.split('/')
            uid = l[-1].split('?')[0]
            if uid not in fans_list:
                fans_list.append(uid)
    # print fans_list

if __name__ == "__main__":
    # uids = [1270739603, 2769807303, 1852651681, 2375055637]
    uids = [1270739603, 2769807303]

    fetcher = Fetcher()
    fetcher.login(username = "***@126.com", pwd = "******")

    db_para = {'host':'localhost', \
               'user':'root',\
               'pwd':'*****',\
               'db':'weibo',\
               'port':3306,\
               'charset':'utf8'}
    db = connectDb(db_para)

    for user in uids:
        print "user: ", user
        get_user(fetcher, user, db)
        time.sleep(2)

    disconnectDb(db)
    print "done"
