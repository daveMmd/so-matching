#include <iostream>
#include <sys/time.h>
#include <unistd.h>
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

    printf("Super charactrer bit num:%d\n", SUPER_BIT_NUM);
    printf("\n");
}

int main(int argc, char** argv) {
    list<string>* patterns = nullptr;
    struct timeval begin, end;

    parse_arguments(argc, argv);
    //if(command.rule_format == nullptr || strcmp(command.rule_format, "clamav") == 0) patterns = read_patterns_from_clamav_ndb(command.pattern_file, command.max_pattern_num);
    if(strcmp(command.rule_format, "clamav") == 0) patterns = read_patterns_from_clamav_ndb(command.pattern_file, command.max_pattern_num);
    else if(strcmp(command.rule_format, "hyperscan") == 0) patterns = read_patterns_from_hyperscan_hex(command.pattern_file, command.max_pattern_num);
    else patterns = read_patterns_from_plaintext(command.pattern_file, command.max_pattern_num);

    gettimeofday(&begin, nullptr);
    auto matching_engine = new soengine(patterns, command.grouping_method);
    gettimeofday(&end, nullptr);
    double diff_seconds = (end.tv_sec - begin.tv_sec) + 0.000001 * (end.tv_usec - begin.tv_usec);
    printf("shift-or enginge construction time: %lf seconds\n", diff_seconds);

    //matching_engine->debug();

    //matching_engine->filtering_effectiveness();

    //matching_engine->show_duplicate_patterns_in_same_bucket();

    matching_engine->output_mif();

    if(command.traffic_file){
        //string* T = read_traffic_from_pcap(command.traffic_file);
        string* T = read_text_from_file(command.traffic_file);
        matching_engine->match(T);
    }

    return 0;
}
