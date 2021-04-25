//
// Created by 钟金诚 on 2021/3/2.
//

#ifndef SO_MATCHING_UTIL_H
#define SO_MATCHING_UTIL_H

#include <string>
#include <list>
using namespace std;

list<string>* read_patterns_from_clamav_ndb(char* fname, int max_pattern_num);
list<string>* read_patterns_from_plaintext(char* fname, int max_pattern_num);
list<string>* read_patterns_from_hyperscan_hex(char* fname, int max_pattern_num);
string* read_traffic_from_pcap(char* fname);
#endif //SO_MATCHING_UTIL_H
