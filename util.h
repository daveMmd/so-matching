//
// Created by 钟金诚 on 2021/3/2.
//

#ifndef SO_MATCHING_UTIL_H
#define SO_MATCHING_UTIL_H

#include <string>
#include <list>
#include <cstdint>
#include <vector>

using namespace std;

typedef struct {
    std::string fastPattern;
    uint16_t    ruleID;
    std::vector<std::string> shortPatterns;
}SnortRule;

list<string>* read_patterns_from_clamav_ndb(char* fname, int max_pattern_num);
list<string>* read_patterns_from_plaintext(char* fname, int max_pattern_num);
list<string>* read_patterns_from_hyperscan_hex(char* fname, int max_pattern_num);
string* read_traffic_from_pcap(char* fname);
string* read_text_from_file(char* fname);
vector<SnortRule> read_snortrules_from_file(char* fname);
#endif //SO_MATCHING_UTIL_H
