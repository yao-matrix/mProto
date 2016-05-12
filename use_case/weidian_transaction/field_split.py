#!/usr/bin/env python
# coding=utf-8

import re

def clean_string(s):
  token = ",.<>?:;{}[]()*&^%$#@~`\'\"《》，。／？“”‘’：；（）＊＆……￥！～".decode('utf-8')
  s1 = [fil for fil in list(s) if fil not in token]
  return ''.join(s1)

def extract_tele(s):
  m = re.search(r'\d{11}', s)
  return m.group(), m.start()

def parse_record(record):
  """
  parse record to structered information
  """
  record_dict = {}
  record = record.decode('utf-8')

  # find tags
  m_idx = record.find(u'型号：')

  r_idx = record.find(u'收件人：')
  if r_idx == -1:
    r_idx = record.find(u'收件地址：')
    
  s_idx = record.find(u'发件人：')
  if s_idx == -1:
    s_idx = record.find(u'寄件人：')

  # handle merchandise segment
  if m_idx == -1:
    m_idx = 0
  else:
    m_idx += 3
  m = record[m_idx:r_idx]
  record_dict['merchandise'] = m

  # handle receiver segment
  rp = None
  ra = None
  re = None
  rt = None

  r = record[r_idx:s_idx]
  # print r

  ## format 1
  rp_idx = r.find(u'收件人：')
  if rp_idx != -1: 
    # print "format 1"
    rt_idx = r.find(u'电话：')
    ra_idx = r.find(u'地址：')
    rp = r[rp_idx + 4:rt_idx]
    rt = r[rt_idx + 3:ra_idx]
    ra = r[ra_idx + 3:]
    rp = clean_string(rp)
    rt = clean_string(rt)
    ra = clean_string(ra)
  ## format 2
  else:
    # print "format 2"
    r = r[5:]
    rw = r.split(u'，')
    ra = clean_string(rw[0])
    re = clean_string(rw[1])
    rt, rt_idx = extract_tele(rw[2])
    if rw[2][rt_idx - 1] == u'收':
      rp = rw[2][0:rt_idx - 1]
    else:
      rp = rw[2][0:rt_idx]
  record_dict['receiver'] = rp
  record_dict['receiver_address'] = ra
  record_dict['receiver_entity'] = re
  record_dict['receiver_telephone'] = rt

  # handle sender segment
  s = record[s_idx + 4:]
  st, st_idx = extract_tele(s)
  sp = s[0:st_idx]
  record_dict['sender'] = sp
  record_dict['sender_telephone'] = st

  print m, rp, ra, re, rt, sp, st
  # print record_dict

if __name__ == '__main__':
  # read record from log
  with open('test.log', 'r') as f:
    i = 1
    for record in f.readlines():
      print "#### RECORD %d ####" % (i)
      parse_record(record.strip())
      i += 1
