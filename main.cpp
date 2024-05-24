#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include "util.h"
#include "soengine.h"

struct _cmd{
    char* pattern_file = nullptr;
    char* traffic_file = nullptr;
    char* rule_format = nullptr;
    int max_pattern_num = 8000;
    int grouping_method;
    bool use_clamav = true; //use clamav or plain-text format rules
}command;

void parse_arguments(int argc, char** argv){
    int opt;

    std::cout << "Hello, Shift-or matching!" << std::endl;
    if(argc < 3) {
        //../../rule-database/clamav-sigs/main.ndb 1000
        printf("usage: %s -p <pattern-file> -n <pattern-number> -f <format> -g <grouping-method:1,2,3> -t <trace-file>\n", argv[0]);
        exit(-1);
    }
    while((opt = getopt(argc, argv, "g:p:n:f:t:")) != -1){
        switch(opt){
            case 'g':
                sscanf(optarg, "%d", &command.grouping_method);
                break;
            case 'p':
                command.pattern_file = optarg;
                break;
            case 't':
                command.traffic_file = optarg;
                break;
            case 'n':
                sscanf(optarg, "%d", &command.max_pattern_num);
                break;
            case 'f':
                //command.use_clamav = false;
                command.rule_format = optarg;
                break;
            case '?':
                fprintf(stderr, "Unkown opt:%c\n", optopt);
                break;
        }
    }

    if(command.pattern_file == nullptr){
        fprintf(stderr, "Please specify pattern-file.\n");
        exit(-1);
    }
    printf("pattern-file:%s\npattern-num:%d\n", command.pattern_file, command.max_pattern_num);
    if(command.rule_format == nullptr) printf("use clamav format rule.\n");
    else printf("use %s format rule.\n", command.rule_format);

    //printf("Super charactrer bit num:%d\n", SUPER_BIT_NUM);
    printf("\n");
}

// 将字符串按长度分割
std::vector<std::string> splitString(const std::string &str, size_t length) {
    std::vector<std::string> result;
    for (size_t i = 0; i < str.size(); i += length) {
        //result.push_back(str.substr(i, length));
        auto tmp_str = str.substr(i, length);
        auto padding_str = tmp_str + std::string(length - tmp_str.length(), '0');

        //reverse
        std::reverse(padding_str.begin(), padding_str.end());

        result.push_back(padding_str);
    }
    return result;
}

// 将字符串分割成长度为32的子串，并以十六进制格式写入文件
void simulation_string2memory(const std::string &str, size_t length = 32, string filename = "./tbmatch.txt") {
    //std::string filename = "./tbmatch.txt";
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    std::vector<std::string> substrings = splitString(str, length);
    for (const auto &substr : substrings) {
        for (const auto &ch : substr) {
            //outfile << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ch) << " ";
            outfile << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ch);
        }
        outfile << std::endl;
    }

    outfile.close();
}

int main(int argc, char** argv) {
    list<string>* patterns = nullptr;
    struct timeval begin, end;

    parse_arguments(argc, argv);

    auto rules = read_snortrules_from_file(command.pattern_file);

    gettimeofday(&begin, nullptr);
    auto matching_engine = new soengine(rules, 5);
    auto matching_engine_NFPSM = new soengine(rules, 6,false);
    gettimeofday(&end, nullptr);
    double diff_seconds = (end.tv_sec - begin.tv_sec) + 0.000001 * (end.tv_usec - begin.tv_usec);
    printf("shift-or enginge construction time: %lf seconds\n", diff_seconds);


    matching_engine->output_mif();
    matching_engine_NFPSM->output_mif();

    if(command.traffic_file){
        //string* T = read_traffic_from_pcap(command.traffic_file);
        string* T = read_text_from_file(command.traffic_file);
        simulation_string2memory(*T);
        simulation_string2memory(*T, 64, "tbpacket.txt");
        matching_engine->co_match(T);
        matching_engine_NFPSM->co_match(T);
    }

    return 0;
}
