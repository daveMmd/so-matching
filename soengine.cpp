//
// Created by 钟金诚 on 2021/3/1.
//

#include "soengine.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <map>

bool lengthcmp(string &s1, string &s2){
    return s1.size() < s2.size();
}

bool double_equal(double a, double b){
    return abs(a-b) < 0.000000001;
}

/*dynamic programming in hyperscan*/
void pattern_grouping(vector<string> *patterns, vector<string>* patterns_in_each_bucket){
    double alpha = 1.05, belta = 3;

    //sorting patterns in the ascending order of their length
    //patterns->sort(lengthcmp);
    sort(patterns->begin(), patterns->end(), lengthcmp);
    vector<string> vec_patterns = *patterns;

    //initialization
    int s = patterns->size();
    int tmp0 = vec_patterns[0].size();
    int tmp1 = vec_patterns[s-1].size();

    auto **t = (double**) malloc(sizeof(double*) * s);
    for(int i = 0; i < s; i++)
        t[i] = (double *) malloc(sizeof(double) * BUCKET_NUM);

    //将编号i到s-1分到一个bucket的cost
    for(int i = 0; i < s; i++) t[i][0] = pow((s - i), alpha) / pow(vec_patterns[i].size(), belta);

    //propagating calculation
    for(int j = 1; j < BUCKET_NUM; j++){
        for(int i = s-1; i >= 0; i--){
            //计算t[i][j]
            //initial mincost = 将i到s-1放置在一个bucket中
            double mincost = pow(s - 1 - i + 1, alpha) / pow(vec_patterns[i].size(), belta);
            //for(int k = i + 1; k < s - 1; k++){
            for(int k = i; k < s - 1; k++){
                mincost = min(mincost, t[k+1][j-1] + pow(k - i + 1, alpha) / pow(vec_patterns[i].size(), belta));
            }
            t[i][j] = mincost;
        }
    }

    //reverse calculate the (BUCKET_NUM-1) division points
    vector<int> division_points;
    //将0到s-1对应的字符串放置于j+1个bucket中的mincost
    double mincost = t[0][BUCKET_NUM - 1];
    int start_point = 0;
    for(int j = BUCKET_NUM - 1; j >= 0; j--){
        for(int i = start_point; i < s; i++){
            //从i处划分bucket区域的cost
            double cost = t[i][j-1] + pow(i - 1 - start_point + 1, alpha) / pow(vec_patterns[start_point].size(), belta);
            if(double_equal(cost, mincost)){
                start_point = i;
                division_points.push_back(start_point);
                mincost = t[i][j-1];
                break;
            }
        }
    }

    if(division_points.size() != BUCKET_NUM - 1) {
        printf("division_points num:%lu\n", division_points.size());
        exit(-1);
    }
    printf("division points:");
    for(int division_point : division_points) printf("%d ", division_point);
    printf("\n\n");

    //put into each bucket patterns
    int start_index = 0;
    division_points.push_back(s);
    for(int i=0; i<BUCKET_NUM; i++){
        int uni_length = vec_patterns[start_index].size();
        for(int j=start_index; j<division_points[i]; j++){
            string uni_length_pattern(vec_patterns[j], 0, uni_length);
            patterns_in_each_bucket[i].push_back(uni_length_pattern);
        }
        start_index = division_points[i];
    }

    //free resources
    for(int i=0; i<s; i++) free(t[i]);
    free(t);
}

/*group in sequence*/
void my_grouping(vector<string> *patterns, vector<string>* patterns_in_each_bucket){
    vector<string> v_patterns = *patterns;
    int ave = patterns->size() / BUCKET_NUM;
    int start_index = 0;
    for(int i = 0; i < BUCKET_NUM; i++){
        for(int j = 0; j < ave; j++){
            patterns_in_each_bucket[i].push_back(v_patterns[start_index + j]);
        }
        start_index += ave;
    }
}

//Pigasus fixed-length grouping:
//pigasus first-filter state 初始值为[63:0] : 0000000000000011000001110000111100011111001111110111111111111111
//-> pigasus 每个bucket长度为固定的: 2, 3, 4, 5, 6, 7, 8, 8
void pigasus_grouping(vector<string> *patterns, vector<string>* patterns_in_each_bucket, int* bucket_pattern_length){
    bucket_pattern_length[0] = 2;
    bucket_pattern_length[1] = 3;
    bucket_pattern_length[2] = 4;
    bucket_pattern_length[3] = 5;
    bucket_pattern_length[4] = 6;
    bucket_pattern_length[5] = 7;
    bucket_pattern_length[6] = 8;
    bucket_pattern_length[7] = 8;
    int cnt_len8 = 0;
    for(const auto& pattern :*patterns){
        auto pattern_len = pattern.size();
        assert(pattern_len >= 2 && pattern_len <= 8);
        if(pattern_len < 8){
            auto bucket_choose = pattern_len - 2;
            patterns_in_each_bucket[bucket_choose].push_back(pattern);
        }
        else{//pattern_len == 8
            //average place to bucket 6,7
            auto bucket_choose = 6 + (cnt_len8++) % 2;
            patterns_in_each_bucket[bucket_choose].push_back(pattern);
        }
    }
}

double calculate_pmatching(vector<string>* patterns_in_each_bucket, int bucket_id){
    vector<string> patterns= patterns_in_each_bucket[bucket_id];
    int pattern_num = patterns.size();
    if(pattern_num <= 0) return 0;
    int minlength = 0xff;
    for(int i = 0; i < pattern_num; i++){
        //if(i == pattern_id) continue;/*exclude pattern_id pattern*/
        if(patterns[i].length() < minlength) minlength = patterns[i].length();
    }

    //case_num[depth][char] 表示深度为depth时末尾字符为c的可能路径数。depth(从左往右数)
    uint64_t case_num[MAX_PATTERN_LENGTH][256];
    for(int depth = 0; depth < minlength; depth++){
        for(int c = 0; c < 256; c++) case_num[depth][c] = 0;
    }

    for(int depth = 0; depth < minlength; depth++){
        if(depth > 0){
            //is_char_in_super_group[group_id][c] 判定 char c是否在group对应的super character group中
            bool is_char_in_super_group[1 << SUPER_BIT_NUM][256];
            for(auto & group : is_char_in_super_group){
                for(bool & is_c_in : group) is_c_in = false;
            }

            //cur_char_exist[c]表示当前depth深度字符c是否存在
            bool cur_char_exist[256];
            for(bool & i : cur_char_exist) i = false;
            for(int i = 0; i < pattern_num; i++){
                //if(i == pattern_id)  continue;/*exclude pattern_id pattern*/
                unsigned char c = patterns[i][depth];
                unsigned char last_c = patterns[i][depth - 1];
                int group = c % (1 << SUPER_BIT_NUM);
                is_char_in_super_group[group][last_c] = true;
                cur_char_exist[c] = true;
            }

            for(int c = 0; c < 256; c++){
                if(!cur_char_exist[c]) {
                    case_num[depth][c] = 0;
                }
                else{
                    int group = c % (1 << SUPER_BIT_NUM);
                    for(int last_c = 0; last_c < 256; last_c++){
                        if(is_char_in_super_group[group][last_c]) case_num[depth][c] += case_num[depth - 1][last_c];
                    }
                }
            }
        }
        //深度为0,所有出现字符c的可能情况只有1种
        else{
            for(int i = 0; i < pattern_num; i++){
                //if(i == pattern_id)  continue;/*exclude pattern_id pattern*/
                unsigned char c = patterns[i][depth];
                case_num[depth][c] = 1;
            }
        }
    }

    //计算总可能数，即深度为pattern_length - 1的所有字符结尾的情况相加
    uint64_t total_cases = 0;
    for(int c = 0; c < 256; c++) total_cases += case_num[minlength - 1][c];
    double pmatching = total_cases * 1.0 / pow(256, minlength);

    return pmatching;
}

double calculate_pmatching_reduce(vector<string>* patterns_in_each_bucket, int bucket_id, int pattern_id){
    double p1 = calculate_pmatching(patterns_in_each_bucket, bucket_id);
    patterns_in_each_bucket[bucket_id].erase(patterns_in_each_bucket[bucket_id].begin() + pattern_id);
    double p2 = calculate_pmatching(patterns_in_each_bucket, bucket_id);
    return p1 - p2;
}

double  calculate_pmatching_add(vector<string>* patterns_in_each_bucket, int bucket_id, string pattern){
    double p1 = calculate_pmatching(patterns_in_each_bucket, bucket_id);
    patterns_in_each_bucket[bucket_id].push_back(pattern);
    double p2 = calculate_pmatching(patterns_in_each_bucket, bucket_id);
    patterns_in_each_bucket[bucket_id].pop_back();
    return p2 - p1;
}

void localsearch_grouping(vector<string> *patterns, vector<string>* patterns_in_each_bucket){
    vector<string> v_patterns = *patterns;

#if 0
    /*1.randomly partition patterns into each group*/
    int ave = patterns->size() / BUCKET_NUM;
    int start_index = 0;
    for(int i = 0; i < BUCKET_NUM; i++){
        for(int j = 0; j < ave; j++){
            patterns_in_each_bucket[i].push_back(v_patterns[start_index + j]);
        }
        start_index += ave;
    }
    //left push into bucket 1
    for(; start_index < patterns->size(); start_index++) patterns_in_each_bucket[0].push_back(v_patterns[start_index]);
#elif 0
    /*1.patterns in the same length are partitioned into one group*/
    for(auto pattern : v_patterns){
        int length = pattern.size();
        patterns_in_each_bucket[length - 1].push_back(pattern);
    }
#elif 0
    /*1.patterns in the same length are partitioned into one group. No empty group*/
    sort(v_patterns.begin(), v_patterns.end(), lengthcmp);
    int cur_bucket_id = 0;
    int consec = 1;
    patterns_in_each_bucket[cur_bucket_id].push_back(v_patterns[0]);
    for(int i = 1; i < v_patterns.size(); i++){
        if(v_patterns[i].size() != v_patterns[i-1].size()){
            cur_bucket_id++;
            consec = 1;
        }
        else if(consec > v_patterns.size() / 8){
            cur_bucket_id++;
            consec = 1;
        }
        else consec++;

        patterns_in_each_bucket[cur_bucket_id].push_back(v_patterns[i]);
    }
#else
    //first use the intial grouping method
    pattern_grouping(patterns, patterns_in_each_bucket);
#endif
    /*local search local-optimal solution*/
    int best_cnt = 0;
    int max_best = patterns->size();
    int max_iter = patterns->size();
#define T1 100
#define T2 20000
    if(max_best > T1) max_best = T1;
    if(max_iter > T2) max_iter = T2;
    //max_iter = 16000;
    int iter_cnt = 0;
    while(1){
        if(iter_cnt % 100 == 0) printf("Iteration %d ...\n", iter_cnt);
        //randomly choose one pattern
        int choose_bucket = rand() % BUCKET_NUM;
        if(!patterns_in_each_bucket[choose_bucket].size()) continue;
        int pattern_id = rand() % patterns_in_each_bucket[choose_bucket].size();
        string pattern = patterns_in_each_bucket[choose_bucket][pattern_id];
        iter_cnt++;
        if(iter_cnt > max_iter) break;

        //calculate the postiveness (reduced matching possibility)
        double p1 = calculate_pmatching_reduce(patterns_in_each_bucket, choose_bucket, pattern_id);

        double minp = p1;
        int moveto_bucket = choose_bucket;

        for(int bucket_id = 0; bucket_id < BUCKET_NUM; bucket_id++){
            if(bucket_id == choose_bucket) continue;
            double p2 = calculate_pmatching_add(patterns_in_each_bucket, bucket_id, pattern);
            if(p2 < minp){
                minp = p2;
                moveto_bucket = bucket_id;
            }
        }
        //do move pattern to target bucket
        patterns_in_each_bucket[moveto_bucket].push_back(pattern);
        if(moveto_bucket == choose_bucket){
            best_cnt++;
            if(best_cnt > max_best) break;
        }
        else best_cnt = 0;
    }
}

/*cut down length larger than MAX_PATTERN_LENGTH*/
vector<string> *cut_length(list<string> *patterns){
    auto *cut_patterns = new vector<string>();

    for(const auto& pattern: *patterns){
        if(pattern.size() > MAX_PATTERN_LENGTH)
        {
            if(!CUT_DIRECTION) cut_patterns->push_back(string(pattern, 0, MAX_PATTERN_LENGTH));
            else cut_patterns->push_back(string(pattern, pattern.length() - MAX_PATTERN_LENGTH, MAX_PATTERN_LENGTH));
        }
        else cut_patterns->push_back(pattern);
    }

    return cut_patterns;
}

void soengine::generate_shiftor_mask(){
    //first initialize all to 1 (1表示状态不活跃)
    for(int c=0; c < (1 << (8 + super_bit_num)); c++) shiftorMasks[c].set();

    for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
        if(patterns_in_each_bucket[bucket].empty()) continue;
        int pattern_length = patterns_in_each_bucket[bucket][0].size();
        //"padding byte": set bit 0 if byte position of sh-or-mask exceeds the longest pattern
        for(int c=0; c < (1 << (8 + super_bit_num)); c++){
            for(int toleft_offset = pattern_length; toleft_offset < MAX_PATTERN_LENGTH; toleft_offset++){
                //shiftorMasks[c][toleft_offset * BUCKET_NUM + bucket] = 0;
                shiftorMasks[c][toleft_offset * BUCKET_NUM + ( 7 - bucket)] = 0;
            }
        }

        //iterate each offset
        for(int toright_offset = 0; toright_offset < pattern_length; toright_offset++){
            //iterate each pattern
            for(int i=0; i<patterns_in_each_bucket[bucket].size(); i++){
                unsigned char c = patterns_in_each_bucket[bucket][i][pattern_length - 1 - toright_offset];
                if(toright_offset == 0){//set (1<<SUPER_BIT_NUM) bits 0
                //if((toleft_offset + 1) == pattern_length){//set (1<<SUPER_BIT_NUM) bits 0
                    for(int fillend = 0; fillend < (1 << super_bit_num); fillend++){
                        //int index = (c << SUPER_BIT_NUM)  + fillend;
                        int index = (fillend << 8) + c;
                        //printf("building, (padding) index : 0x%x\n", index);
                        //shiftorMasks[index][toleft_offset * BUCKET_NUM + bucket] = 0;
                        shiftorMasks[index][toright_offset * BUCKET_NUM + (7 - bucket)] = 0;
                    }
                }
                else{//set one bit 0
                    unsigned char next_c = patterns_in_each_bucket[bucket][i][pattern_length - 1 - toright_offset + 1];
                    //int index = (c << SUPER_BIT_NUM) + next_c % (1 << SUPER_BIT_NUM);
                    int index = ((next_c % (1 << super_bit_num)) << 8) + c;
                    //printf("building, index : 0x%x\n", index);
                    //shiftorMasks[index][toleft_offset * BUCKET_NUM + bucket] = 0;
                    shiftorMasks[index][toright_offset * BUCKET_NUM + (7 - bucket)] = 0;
                }
            }
        }
    }
}

soengine::soengine(list<string> *patterns, int grouping_method) {
    vector<string> *cut_patterns = cut_length(patterns);

    //pattern grouping
    if(grouping_method == 1)
    {
        printf("Using hyperscan-dynamic grouping\n");
        pattern_grouping(cut_patterns, patterns_in_each_bucket);
    }
    else if(grouping_method == 0){
        printf("Using average grouping\n");
        my_grouping(cut_patterns, patterns_in_each_bucket);
    }
    else if(grouping_method == 2) {
        printf("Using local-search grouping\n");
        localsearch_grouping(cut_patterns, patterns_in_each_bucket);
    }
    else if(grouping_method == 3){
        printf("Using Pigasus fixed-length grouping method!\n");
        //todo
        pigasus_grouping(cut_patterns, patterns_in_each_bucket, bucket_pattern_length);
    }
    else{
        printf("wrong grouping method!\n");
        exit(-1);
    }

    //generate shift-or masks for each character
    generate_shiftor_mask();

    generate_bitmap();
}

void soengine::debug() {
    FILE* f = fopen("./shiftor_masks.txt", "w");
    for(int c=0; c < (1 << (8 + super_bit_num)); c++){
        fprintf(f, "%d:", c);
        string maskstr = shiftorMasks[c].to_string();
        for(int length = 0; length < MAX_PATTERN_LENGTH; length++){
                fprintf(f, ",%s", string(maskstr, length * BUCKET_NUM, BUCKET_NUM).c_str());
        }
        fprintf(f, "\n");
    }

    fclose(f);
}

double soengine::filtering_effectiveness() {

    double sum_pmatching = 0;
    //first: 对每个bucket计算所有可能匹配字符串
    for(int bucket = 0; bucket < BUCKET_NUM; bucket++) {
        vector<string> patterns= patterns_in_each_bucket[bucket];
        int pattern_num = patterns.size();
        if(pattern_num <= 0) continue;
        int pattern_length = patterns[0].size();
        for(int i = 1; i < pattern_num; i++){
            if(patterns[i].size() < pattern_length) pattern_length = patterns[i].size();
        }

        //case_num[depth][char] 表示深度为depth时末尾字符为c的可能路径数。depth(从左往右数)
        uint64_t **case_num = (uint64_t **) malloc(sizeof(uint64_t *) * pattern_length);
        for(int depth = 0; depth < pattern_length; depth++){
            case_num[depth] = (uint64_t *) malloc(sizeof(uint64_t) * 256);
            for(int c = 0; c < 256; c++) case_num[depth][c] = 0;
        }

        for(int depth = 0; depth < pattern_length; depth++){
            if(depth > 0){
                //is_char_in_super_group[group_id][c] 判定 char c是否在group对应的super character group中
                bool is_char_in_super_group[1 << super_bit_num][256];
                for(auto & group : is_char_in_super_group){
                    for(bool & is_c_in : group) is_c_in = false;
                }

                //cur_char_exist[c]表示当前depth深度字符c是否存在
                bool cur_char_exist[256];
                for(bool & i : cur_char_exist) i = false;
                for(int i = 0; i < pattern_num; i++){
                    unsigned char c = patterns[i][depth];
                    unsigned char last_c = patterns[i][depth - 1];
                    int group = c % (1 << super_bit_num);
                    is_char_in_super_group[group][last_c] = true;
                    cur_char_exist[c] = true;
                }

                for(int c = 0; c < 256; c++){
                    if(!cur_char_exist[c]) {
                        case_num[depth][c] = 0;
                    }
                    else{
                        int group = c % (1 << super_bit_num);
                        //for(int last_c = group; last_c < 256; last_c += (1 << SUPER_BIT_NUM)){
                        for(int last_c = 0; last_c < 256; last_c++){
                            if(is_char_in_super_group[group][last_c]) case_num[depth][c] += case_num[depth - 1][last_c];
                        }
                    }
                }
            }
            //深度为0,所有出现字符c的可能情况只有1种
            else{
                for(int i = 0; i < pattern_num; i++){
                    unsigned char c = patterns[i][depth];
                    case_num[depth][c] = 1;
                }
            }

            //打印到达深度为depth时所有可能匹配路径数
            if(DEBUG_MATCHING_CASES){
                uint64_t total_cases = 0;
                for(int c = 0; c < 256; c++) total_cases += case_num[depth][c];
                printf("depth-%d possible matching cases: %llu\n", depth, total_cases);
            }
        }

        //计算总可能数，即深度为pattern_length - 1的所有字符结尾的情况相加
        uint64_t total_cases = 0;
        for(int c = 0; c < 256; c++) total_cases += case_num[pattern_length - 1][c];
        printf("bucket %d possible matching cases: %llu\n", bucket, total_cases);
        //printf("bucket %d matching possibility:%llf\n", bucket, total_cases * 1.0 / (1 << (8 * pattern_length)));
        printf("log2(x): %f\n", log(total_cases * 1.0) / log(2));
        sum_pmatching += total_cases * 1.0 / pow(256, pattern_length);
        //free
        for(int i=0; i<pattern_length; i++) free(case_num[i]);
        free(case_num);
    }

    printf("Sum of pmatching:%f (%e)\n", sum_pmatching, sum_pmatching);
    return 0;
}

void soengine::show_duplicate_patterns_in_same_bucket() {
    for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
        vector<string> patterns = patterns_in_each_bucket[bucket];
        int pattern_num = patterns.size();
        if(pattern_num <= 0) continue;
        int pattern_length = patterns[0].size();

        int duplicate_num = 0;
        for(int i = 1; i < pattern_num; i++){
            for(int j = 0; j < i; j++){
                if(patterns[i] == patterns[j]) {
                    duplicate_num++;
                    break;
                }
            }
        }

        if(duplicate_num) printf("bucket %d duplicate pattern num: %d\n", bucket, duplicate_num);
    }
}


#define MATCH_DEBUG 2
void soengine::match(string *T) {
    /*初始化state-mask*/
    bitset<2 * MAX_PATTERN_LENGTH * BUCKET_NUM> state_mask;
    state_mask.reset(); //state-mask 所有位置为0
    //每一个bucket，小于pattern-length的位置置为1，避免误匹配
    //pigasus first-filter state 初始值为[63:0] : 0000000000000011000001110000111100011111001111110111111111111111
    //-> pigasus 每个bucket长度为固定的: 2, 3, 4, 5, 6, 7, 8, 8 (hash table对应长度一致)
    //pigasus NFPS 模块 init state 初始值为[63:0]: 0000000000000011000001110000111100011111001111110111111111111111
    //每个bucket长度为固定的: 2, 3, 4, 5, 6, 7, 8, 8
    //可是... hash table 对应的长度却分别为1,2,3,4,5,6,7,8...
    for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
        int pattern_length;
        if(patterns_in_each_bucket[bucket].size() < 1) {
            //continue;
            pattern_length = bucket_pattern_length[bucket];
        }
        else pattern_length = patterns_in_each_bucket[bucket][0].size();

        for(int length = 0; length < pattern_length - 1; length++){
            //state_mask[length * BUCKET_NUM + bucket] = 1;
            state_mask[length * BUCKET_NUM + (BUCKET_NUM - 1 - bucket)] = 1;
        }
    }

    //print:    0000000000000011000001110000111100011111001111110111111111111111
    //stt-mask: 0000000000000011000001110000111100011111001111110111111111111111
    if(MATCH_DEBUG <= 1) std::cout << "init stt_mask: " << state_mask.to_string() << endl;

    string Text = *T;
    int match_num = 0;
    int match_num_each_length = 0; //每一个位置不同bucket的匹配只计数一次
    //一次匹配MAX_PATTERN_LENGTH个字节
    int pos;
    for(pos = 0; pos + MAX_PATTERN_LENGTH < Text.length(); pos += MAX_PATTERN_LENGTH){
        so_mask_t so_masks[8];

        //读shift-or-mask
        for(int i = 0; i < MAX_PATTERN_LENGTH; i++){
            unsigned char c1 = Text[pos + i];
            unsigned char c2 = Text[pos + i + 1]; //可能超出边界
            //int index =  c1 * (1 << SUPER_BIT_NUM) + c2 % (1 << SUPER_BIT_NUM);
            int index = ((c2 % (1 << super_bit_num)) << 8) + c1;
            so_masks[i].reset(); //初始化为全0
            //cout << "index: " << std::hex << index << " so-mask[]: " << shiftorMasks[index].to_string() << endl;
            //so_masks[i] = shiftorMasks[index];
            for(int length = 0; length < MAX_PATTERN_LENGTH; length++){
                for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
                    so_masks[i][length * BUCKET_NUM + bucket] = shiftorMasks[index][length * BUCKET_NUM + bucket];
                }
            }
            so_masks[i] = so_masks[i] << (i * BUCKET_NUM);

            if(MATCH_DEBUG <= 1) std::cout << "so_mask" << i << ": " << so_masks[i].to_string() << endl;
        }

        //shift-or运算
        for(int i = 0; i < MAX_PATTERN_LENGTH; i++){
            state_mask |= so_masks[i];
        }
        if(MATCH_DEBUG <= 1) std::cout << "state_mask: " << state_mask.to_string() << endl;

        //检测是否存在匹配:小于MAX_PATTERN_LENGTH的位置是否存在0 bit
        for(int length = 0; length < MAX_PATTERN_LENGTH; length++){
            bool match_flag = false;
            for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
                if(state_mask[length * BUCKET_NUM + bucket] == 0){
                    if(MATCH_DEBUG <= 2) printf("position %d match patterns in bucket %d\n", pos + length, (7 - bucket));
                    match_num++;
                    if(!match_flag){
                        match_num_each_length++;
                        match_flag = true;
                    }
                }
            }
        }

        state_mask >>= (MAX_PATTERN_LENGTH * BUCKET_NUM);
    }

    //匹配末尾几个字节
    int left_length = Text.length() - pos;
    so_mask_t so_masks[8];

    //读shift-or-mask
    for(int i = 0; i < left_length; i++){
        unsigned char c1 = Text[pos + i];
        unsigned char c2 = Text[pos + i + 1]; //可能超出边界
        //int index =  c1 * (1 << SUPER_BIT_NUM) + c2 % (1 << SUPER_BIT_NUM);
        int index = ((c2 % (1 << super_bit_num)) << 8) + c1;
        so_masks[i].reset(); //初始化为全0
        //so_masks[i] = shiftorMasks[index];
        for(int length = 0; length < MAX_PATTERN_LENGTH; length++){
            for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
                so_masks[i][length * BUCKET_NUM + bucket] = shiftorMasks[index][length * BUCKET_NUM + bucket];
            }
        }
        so_masks[i] = so_masks[i] << (i * BUCKET_NUM);

        if(MATCH_DEBUG <= 1) std::cout << "so_mask" << i << ": " << so_masks[i].to_string() << endl;
    }

    //shift-or运算
    for(int i = 0; i < left_length; i++){
        state_mask |= so_masks[i];
    }
    if(MATCH_DEBUG <= 1) std::cout << "stt_mask: " << state_mask.to_string() << endl;

    //检测是否存在匹配:小于MAX_PATTERN_LENGTH的位置是否存在0 bit
    for(int length = 0; length < left_length; length++){
        bool match_flag = false;
        for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
            if(state_mask[length * BUCKET_NUM + bucket] == 0){
                //if(MATCH_DEBUG <= 2) printf("position %d match patterns in bucket %d\n", pos + length, bucket);
                if(MATCH_DEBUG <= 2) printf("position %d match patterns in bucket %d\n", pos + length, (7 - bucket));
                match_num++;
                if(!match_flag){
                    match_num_each_length++;
                    match_flag = true;
                }
            }
        }
    }

    //report
    printf("match_num:%d\nmatch_num_each_length:%d\n", match_num, match_num_each_length);
}

void soengine::output_mif() {
    //call 8 hashtable_bitmap
    for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
        //hashtable_bitmap_eachbucket[bucket]->output_mif("bitmap" + std::to_string(bucket) + ".mif", "hashtable" + std::to_string(bucket) + ".mif");
        if(fpsm_or_nfpsm){ //FPSM
            hashtable_bitmap_eachbucket[bucket]->bitmap_mif_write("bitmap" + std::to_string(bucket) + ".mif");
            hashtable_bitmap_eachbucket[bucket]->hashtable_mif_write("hashtable" + std::to_string(bucket) + ".mif");
        }
        else{ //NFPSM
            hashtable_bitmap_eachbucket[bucket]->bitmap_mif_write("nf_bitmap" + std::to_string(bucket) + ".mif");
        }
    }


    // 将数组写入 .mif 文件的函数
    string filename;
    if(fpsm_or_nfpsm) {
        filename = "./match_table.mif";
    }
    else filename = "./shift_or.mif"; //NFPSM

    std::ofstream mifFile(filename);
    if (!mifFile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    // 写入 MIF 文件头
    mifFile << "WIDTH=64;\n";
    mifFile << "DEPTH=" << (1<<(8 + super_bit_num)) << ";\n\n";
    mifFile << "ADDRESS_RADIX=HEX;\n";
    mifFile << "DATA_RADIX=HEX;\n\n";
    mifFile << "CONTENT BEGIN\n";

    // 写入数组内容
    //cout << "debug" << shiftorMasks[0].to_string() << endl;
    //cout << "debug" << shiftorMasks[0].to_ullong() << endl;

    for (size_t i = 0; i < (1<<(8 + super_bit_num)); ++i) {
        mifFile << "    " << std::setw(4) << std::setfill('0') << std::hex << i << " : "
                << std::setw(2) << std::setfill('0') << std::hex << shiftorMasks[i].to_ullong() << ";\n";
    }

    mifFile << "END;\n";

    mifFile.close();
    std::cout << "MIF file written successfully to " << filename << std::endl;
}

void soengine::generate_bitmap() {
    for(auto bucket = 0; bucket < BUCKET_NUM; bucket++){
        if(fpsm_or_nfpsm){
            //how many bytes should be masked when hashing
            int lsb_bytes_masked = (8 - bucket_pattern_length[7- bucket]);
            if(bucket < 2) hashtable_bitmap_eachbucket[bucket] = new Pigasus::Hashtable_bitmap(15, 16, lsb_bytes_masked, 32768);
            else if(bucket < 4) hashtable_bitmap_eachbucket[bucket] = new Pigasus::Hashtable_bitmap(12, 16, lsb_bytes_masked, 4096);
            else if(bucket == 4) hashtable_bitmap_eachbucket[bucket] = new Pigasus::Hashtable_bitmap(11, 16, lsb_bytes_masked, 2048);
            else if(bucket == 5) hashtable_bitmap_eachbucket[bucket] = new Pigasus::Hashtable_bitmap(12, 16, lsb_bytes_masked, 4096);
            else if(bucket == 6) hashtable_bitmap_eachbucket[bucket] = new Pigasus::Hashtable_bitmap(10, 16, lsb_bytes_masked, 1024);
            else if(bucket == 7) hashtable_bitmap_eachbucket[bucket] = new Pigasus::Hashtable_bitmap(8, 16, lsb_bytes_masked, 256);
        }
        else{ //NFPSM
            int lsb_bytes_masked = (8 - bucket_pattern_length[7- bucket]);
            if(bucket >= 1) lsb_bytes_masked++; //8,7,6,5,4,3,2,1   previous: 8,8,7,6,5,4,3,2

            if(bucket == 0) hashtable_bitmap_eachbucket[bucket] = new Pigasus::Hashtable_bitmap(17, 16, lsb_bytes_masked, 1);
            else if(bucket == 1) hashtable_bitmap_eachbucket[bucket] = new Pigasus::Hashtable_bitmap(13, 16, lsb_bytes_masked, 1);
            else if(bucket == 2) hashtable_bitmap_eachbucket[bucket] = new Pigasus::Hashtable_bitmap(13, 16, lsb_bytes_masked, 1);
            else if(bucket == 3) hashtable_bitmap_eachbucket[bucket] = new Pigasus::Hashtable_bitmap(13, 16, lsb_bytes_masked, 1);
            else if(bucket == 4) hashtable_bitmap_eachbucket[bucket] = new Pigasus::Hashtable_bitmap(13, 16, lsb_bytes_masked, 1);
            else if(bucket == 5) hashtable_bitmap_eachbucket[bucket] = new Pigasus::Hashtable_bitmap(12, 16, lsb_bytes_masked, 1);
            else if(bucket == 6) hashtable_bitmap_eachbucket[bucket] = new Pigasus::Hashtable_bitmap(11, 16, lsb_bytes_masked, 1);
            else if(bucket == 7) hashtable_bitmap_eachbucket[bucket] = new Pigasus::Hashtable_bitmap(10, 16, lsb_bytes_masked, 1);
        }
    }

    //pass map to each bucket Hash_bitmap
    for(int i = 0; i < 8; i++){
        if(fpsm_or_nfpsm){
            hashtable_bitmap_eachbucket[i]->fastPatternToRuleIDMap = &fastPatternToRuleIDMap;
        }
        else{ //NFPSM
            hashtable_bitmap_eachbucket[i]->fastPatternToRuleIDMap = nullptr;
        }
    }


    for(auto bucket = 0; bucket < BUCKET_NUM; bucket++){
        hashtable_bitmap_eachbucket[bucket]->build(patterns_in_each_bucket[7 - bucket]);
    }
}

void soengine::co_match(string *T){
    match(T); //shift-or

    //hash_bitmap match
    for(int i = 0; i < T->size(); i++){
        int start_pos = (i >= 7) ? i - 7 : 0;
        int char_n = (i >= 7) ? 8 : (i + 1);
        string sub_str = T->substr(start_pos, char_n);
        for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
            bool if_match = hashtable_bitmap_eachbucket[bucket]->lookup(sub_str);
            if(if_match){
                printf("Pigasus-hash-bitmap (bucket %d) matched at pos: %d\n", (7 - bucket), i);
            }
        }
    }

}

void soengine::record_pattern_sid_map(vector<SnortRule> &rules){
    //std::map<std::string, uint16_t > fastPatternToRuleIDMap;
    for (const auto &rule : rules) {
        if (!rule.fastPattern.empty()) {
            fastPatternToRuleIDMap[rule.fastPattern] = rule.ruleID;
        }
    }
}

void cut_pattern_length(vector<SnortRule>& rules){

    for(auto& rule: rules){
        auto pattern = rule.fastPattern;
        string cut_pattern;
        if(pattern.size() > MAX_PATTERN_LENGTH)
        {
            if(!CUT_DIRECTION) cut_pattern = string(pattern, 0, MAX_PATTERN_LENGTH);
            else cut_pattern = string(pattern, pattern.length() - MAX_PATTERN_LENGTH, MAX_PATTERN_LENGTH);
            rule.fastPattern = cut_pattern;
        }
        //else cut_pattern = pattern;
    }

    for(auto &rule : rules){
        vector<string> tmp_shortPatterns;
        for(auto &shortPattern: rule.shortPatterns){
            string cut_short_pattern;
            if(shortPattern.size() > MAX_PATTERN_LENGTH)
            {
                if(!CUT_DIRECTION) cut_short_pattern = string(shortPattern, 0, MAX_PATTERN_LENGTH);
                else cut_short_pattern = string(shortPattern, shortPattern.length() - MAX_PATTERN_LENGTH, MAX_PATTERN_LENGTH);
                tmp_shortPatterns.push_back(cut_short_pattern);
            }
            else tmp_shortPatterns.push_back(shortPattern);
        }
        rule.shortPatterns = tmp_shortPatterns;
    }
}

soengine::soengine(vector<SnortRule> rules, int _super_bit_num, bool _fpsm_or_nfpsm) {
    super_bit_num = _super_bit_num;
    shiftorMasks = new shiftor_mask_t[1 << (8 + super_bit_num)];
    fpsm_or_nfpsm = _fpsm_or_nfpsm;

    cut_pattern_length(rules);

    record_pattern_sid_map(rules);

    vector<string> patterns;
    if(fpsm_or_nfpsm){ //FPSM
        for(auto rule : rules){
            patterns.push_back(rule.fastPattern);
        }
    }
    else{//NFPSM
        for(auto rule : rules){
            for(auto short_pattern: rule.shortPatterns){
                if(std::find(patterns.begin(), patterns.end(), short_pattern) == patterns.end()){
                    patterns.push_back(short_pattern);
                }
            }
        }
    }

    //pattern grouping
    printf("Using Pigasus fixed-length grouping method!\n");

    pigasus_grouping(&patterns, patterns_in_each_bucket, bucket_pattern_length);


    //generate shift-or masks for each character
    generate_shiftor_mask();

    generate_bitmap();

    if(fpsm_or_nfpsm == false){ //NFPSM
        generate_fingerprints(rules); //todo
    }
}

bitset<128> combine_bitsets(bitset<16> *fingerprint){
    std::bitset<128> combined;
    // 拼接所有 bitset<16> 到 bitset<128>
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 16; ++j) {
            combined[i * 16 + j] = fingerprint[i][j];
        }
    }

    return combined;
}

void out_fp_mif(vector<bitset<128>> &data){
    string filename = "./rule_fp.mif";
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    size_t maxSize = 8192;
    size_t dataSize = data.size();
    size_t bitWidth = 128;

    // 写入 MIF 文件头
    outfile << "WIDTH=" << bitWidth << ";\n";
    outfile << "DEPTH=" << maxSize << ";\n\n";
    outfile << "ADDRESS_RADIX=HEX;\n";
    outfile << "DATA_RADIX=BIN;\n\n";
    outfile << "CONTENT BEGIN\n";

    // 写入数据
    //padding head
    outfile << std::hex << 0 << " : " << string(128, '0') << ";\n";

    for (size_t i = 0; i < dataSize; ++i) {
        outfile << std::hex << i+1 << " : " << data[i] << ";\n";
    }
    //padding remaining
    for(size_t i = dataSize + 1; i < maxSize; i++){
        outfile << std::hex << i << " : " << string(128, '0') << ";\n";
    }

    outfile << "END;\n";
    outfile.close();
}

void soengine::generate_fingerprints(vector<SnortRule> &rules) {

    vector<bitset<128>> fingerprints;

    for(auto &rule: rules){
        bitset<16> fingerprint[BUCKET_NUM];
        for(int i = 0; i < 8; i++) fingerprint[i].reset(); //init to all 0s
        for(auto &shortPattern: rule.shortPatterns){
            int bucket_choose = BUCKET_NUM - shortPattern.size();
            auto addr = hashtable_bitmap_eachbucket[bucket_choose]->dave_hash(shortPattern);
            auto fp_index = addr % 16;
            fingerprint[bucket_choose].set(fp_index); //endian may be wrong, both for bucket and fp_index
        }
        fingerprints.push_back(combine_bitsets(fingerprint));
    }

    out_fp_mif(fingerprints);
}
