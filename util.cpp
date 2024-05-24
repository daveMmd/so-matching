//
// Created by 钟金诚 on 2021/3/2.
//
#include "util.h"
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <regex>
#include <set>

/*check if all chars are hex digits*/
bool is_exact_string(string &pattern){
    for(auto c: pattern){
        if( c >= '0' && c <= '9') continue;
        if( c >= 'a' && c <= 'f') continue;
        return false;
    }
    return true;
}

int hextoint(char c){
    if(c >= '0' && c <= '9') return c - '0';
    else return c - 'a' + 10;
}

string hextostr(string &hex_pattern){
    if(hex_pattern.size() % 2) {
        fprintf(stderr, "hex_pattern with odd char num!\n");
        cout << hex_pattern << endl;
        exit(-1);
    }
    string pattern;
    for(int i=1; i<hex_pattern.size(); i+=2){
        unsigned char c = hextoint(hex_pattern[i-1]) * 16 + hextoint(hex_pattern[i]);
        pattern.push_back(c);
    }
    return pattern;
}

std::vector<SnortRule> parseSnortRules(const std::string &filename) {
    std::vector<SnortRule> rules;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return rules;
    }

    std::string line;
    std::regex ruleIDRegex(R"(sid:\s*(\d+);)");
    std::regex fastPatternRegex(R"(fast_pattern:\s*\"(.*?)\")");

    while (std::getline(file, line)) {
        std::smatch match;
        SnortRule rule;

        // 提取 ruleID
        if (std::regex_search(line, match, ruleIDRegex) && match.size() > 1) {
            rule.ruleID = std::stoi(match.str(1));
        }

        // 提取 fast_pattern
        if (std::regex_search(line, match, fastPatternRegex) && match.size() > 1) {
            rule.fastPattern = match.str(1);
        }

        if (!rule.fastPattern.empty()) {
            rules.push_back(rule);
        }
    }

    file.close();
    return rules;
}

// 将十六进制字符串转换为字符
std::string hexToString(const std::string& hex) {
    std::string result;
    std::istringstream hexStream(hex);
    std::string byte;

    while (hexStream >> byte) {
        char ch = static_cast<char>(std::stoi(byte, nullptr, 16));
        result += ch;
    }

    return result;
}

// 转换包含十六进制标识的字符串
std::string convertHexInString(const std::string& input) {
    std::regex hexPattern(R"(\|([0-9A-Fa-f\s]+)\|)");
    std::string result = input;
    std::smatch match;

    // 迭代查找并替换所有匹配的十六进制标识
    while (std::regex_search(result, match, hexPattern)) {
        std::string hexString = match.str(1); // 提取十六进制部分
        std::string asciiString = hexToString(hexString); // 转换为ASCII字符
        result.replace(match.position(0), match.length(0), asciiString); // 替换原始匹配部分
    }

    return result;
}

std::vector<SnortRule> parseSnortRules_full(const std::string &filename) {
    std::vector<SnortRule> rules;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return rules;
    }

    std::string line;
    std::regex ruleIDRegex(R"(sid:\s*(\d+);)");
    //std::regex fastPatternRegex(R"(fast_pattern:\s*\"(.*?)\")");
    //std::regex fastPatternRegex(R"(content:\s*\"(.*?)\"\s*,.*fast_pattern.*?;)");
    std::regex fastPatternRegex(R"(content:\s*\"([^\"]*?)\"[^;]*,[^;]*?fast_pattern[^;]*?;)");
    std::regex shortPatternRegex(R"(content:\s*\"(.*?)\".*?;)");

    int compress_sid = 1;
    std::set<string> fastPattern_set = {};
    while (std::getline(file, line)) {
        std::smatch match;
        SnortRule rule;

        // 提取 ruleID
        if (std::regex_search(line, match, ruleIDRegex) && match.size() > 1) {
            rule.ruleID = std::stoi(match.str(1));
            rule.ruleID = compress_sid++;
        }

        // 提取 fast_pattern
        if (std::regex_search(line, match, fastPatternRegex) && match.size() > 1) {
            std::string content = match.str(1);
            auto nohex_content = convertHexInString(content);
            if(nohex_content.size() < 2){
                cerr << "error: fast_pattern size < 2" <<endl;
                exit(-1);
            }
            if(fastPattern_set.find(nohex_content) != fastPattern_set.end()){
                //fprintf(stderr, "error: same fast_pattern :%s", nohex_content);
                cerr << "error: same fast_pattern :" << nohex_content << endl;
                exit(-1);
            }
            fastPattern_set.insert(nohex_content);
            rule.fastPattern = nohex_content;
        }

        // 提取 shortPatterns
        auto shortPatternBegin = std::sregex_iterator(line.begin(), line.end(), shortPatternRegex);
        auto shortPatternEnd = std::sregex_iterator();

        for (std::sregex_iterator i = shortPatternBegin; i != shortPatternEnd; ++i) {
            std::smatch match = *i;
            std::string content = match.str(1);
            auto nohex_content = convertHexInString(content);
            if(nohex_content.size() < 2) continue;
            if(content != rule.fastPattern) rule.shortPatterns.push_back(nohex_content);
        }

        if (!rule.fastPattern.empty()) {
            rules.push_back(rule);
        }
    }

    file.close();
    return rules;
}

vector<SnortRule> read_snortrules_from_file(char* fname){
    return parseSnortRules_full(fname);
}

list<string>* read_patterns_from_clamav_ndb(char* fname, int max_pattern_num){
    FILE* f = fopen(fname, "r");
    auto *patterns = new list<string>();

    char ln[5005];
    while(fgets(ln, 5000, f) != nullptr){
        if(strlen(ln) > 4990){
            fprintf(stderr, "some pattern has more than 4990 chars!!\n");
            printf("%s\n", ln);
            exit(-1);
        }

        int semicolon_cnt = 0;
        int i;
        for(i=0; i<strlen(ln); i++){
            if(ln[i] == ':') {
                semicolon_cnt++;
                if(semicolon_cnt >= 3) break;
            }
        }
        i++;
        if(semicolon_cnt < 2){
            fprintf(stderr, "rule format error!\n");
            printf("%s\n", ln);
            exit(-1);
        }

        int adjust = 0;
        if(ln[strlen(ln)-1] == '\n') adjust = -1;
        string hex_pattern(ln + i, ln + strlen(ln) + adjust);
        //check if pattern is exact string
        if(is_exact_string(hex_pattern)) {
            string pattern = hextostr(hex_pattern);
            patterns->push_back(pattern);
            if(patterns->size() >= max_pattern_num) break;
        }
    }

    fclose(f);

    printf("read patterns number: %lu\n", patterns->size());
    return patterns;
}

unsigned char hex2char(char c1, char c2){
    unsigned char c;
    if(c1 >= '0' && c1 <= '9') c = (c1 - '0') << 4;
    else c = (c1 - 'a' + 10) << 4;
    if(c2 >= '0' && c2 <= '9') c += (c2 - '0');
    else c += c2 - 'a' + 10;
    return c;
}

list<string>* read_patterns_from_hyperscan_hex(char* fname, int max_pattern_num){
    FILE* f = fopen(fname, "r");
    auto *patterns = new list<string>();

    char ln[1005];
    while(fgets(ln, 1000, f) != nullptr){
        if(strlen(ln) > 990){
            fprintf(stderr, "some pattern has more than 990 chars!!\n");
            printf("%s\n", ln);
            exit(-1);
        }

        string pattern = "";
        bool start = false;
        for(int i = 0; i < strlen(ln); i++){
            if(ln[i] == '/' && !start) start = true;
            else if(ln[i] == '/') break;
            else if(start){
                if(ln[i] != '\\') printf("error: ln[i] != \\");
                if(ln[i+1] != 'x') printf("error: ln[i] != x");
                pattern += hex2char(ln[i+2], ln[i+3]);
                i += 3;
            }
        }

        patterns->push_back(pattern);
        if(patterns->size() >= max_pattern_num) break;
    }

    fclose(f);

    printf("read patterns number: %lu\n", patterns->size());
    return patterns;
}

list<string>* read_patterns_from_plaintext(char* fname, int max_pattern_num){
    FILE* f = fopen(fname, "r");
    auto *patterns = new list<string>();

    char ln[1005];
    while(fgets(ln, 1000, f) != nullptr){
        if(strlen(ln) > 990){
            fprintf(stderr, "some pattern has more than 990 chars!!\n");
            printf("%s\n", ln);
            exit(-1);
        }

        int adjust = 0;
        if(ln[strlen(ln)-1] == '\n') adjust = -1;
        string pattern(ln, ln + strlen(ln) + adjust);
        patterns->push_back(pattern);
        if(patterns->size() >= max_pattern_num) break;
    }

    fclose(f);

    printf("read patterns number: %lu\n", patterns->size());
    return patterns;
}

string* read_traffic_from_pcap(char* fname){
    ifstream ftraffic(fname, ios::binary);
    ftraffic.seekg(0, ios::end);
    int length = ftraffic.tellg();
    ftraffic.seekg(0, ios::beg);

    char *traffic = (char*) malloc(sizeof(char) * length);
    ftraffic.read(traffic, length);
    string* T = new string(traffic, traffic+length);

    ftraffic.close();
    free(traffic);

    printf("input traffic size: %lu\n", T->size());
    return T;
}

bool isFileType(const char* fname, const std::string& extension) {
    std::string filename(fname);
    if (filename.length() >= extension.length()) {
        return (0 == filename.compare(filename.length() - extension.length(), extension.length(), extension));
    }
    return false;
}

string* read_traffic_from_txt(char* fname){
    std::ifstream file(fname);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << fname << std::endl;
        return nullptr;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf(); // 读取文件内容到 buffer
    return new std::string(buffer.str()); // 动态分配 std::string 并返回指针
}

string* read_text_from_file(char* fname){
    if(isFileType(fname, ".pcap")){
        return read_traffic_from_pcap(fname);
    }
    else if(isFileType(fname, ".txt")){
        return read_traffic_from_txt(fname);
    }
    else{
        fprintf(stderr, "traffic file type not match!\n");
        return nullptr;
    }
}