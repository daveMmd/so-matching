//
// Created by 钟金诚 on 2021/3/1.
//

#include "soengine.h"
#include <cmath>
#include <iostream>

bool lengthcmp(string &s1, string &s2){
    return s1.size() > s2.size();
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

    /*local search local-optimal solution*/
    int best_cnt = 0;
    int max_best =  patterns->size();
    if(max_best > 1000) max_best = 1000;
    int iter_cnt = 0;
    while(1){
        if(iter_cnt % 100 == 0) printf("Iteration %d ...\n", iter_cnt);
        iter_cnt++;
        //randomly choose one pattern
        int choose_bucket = rand() % BUCKET_NUM;
        int pattern_id = rand() % patterns_in_each_bucket[choose_bucket].size();
        string pattern = patterns_in_each_bucket[choose_bucket][pattern_id];

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
    for(int c=0; c < (1 << (8 + SUPER_BIT_NUM)); c++) shiftorMasks[c].set();

    for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
        if(patterns_in_each_bucket[bucket].empty()) continue;
        int pattern_length = patterns_in_each_bucket[bucket][0].size();
        //"padding byte": set bit 0 if byte position of sh-or-mask exceeds the longest pattern
        for(int c=0; c < (1 << (8 + SUPER_BIT_NUM)); c++){
            for(int toleft_offset = pattern_length; toleft_offset < MAX_PATTERN_LENGTH; toleft_offset++){
                shiftorMasks[c][toleft_offset * BUCKET_NUM + bucket] = 0;
            }
        }

        //iterate each offset
        for(int toleft_offset = 0; toleft_offset < pattern_length; toleft_offset++){
            //iterate each pattern
            for(int i=0; i<patterns_in_each_bucket[bucket].size(); i++){
                unsigned char c = patterns_in_each_bucket[bucket][i][pattern_length - 1 - toleft_offset];
                if(toleft_offset == 0){//set (1<<SUPER_BIT_NUM) bits 0
                    for(int fillend = 0; fillend < (1 << SUPER_BIT_NUM); fillend++){
                        int index = (c << SUPER_BIT_NUM)  + fillend;
                        //shiftorMasks[index][(MAX_PATTERN_LENGTH - toleft_offset - 1) * BUCKET_NUM + (BUCKET_NUM - bucket)] = 1;
                        shiftorMasks[index][toleft_offset * BUCKET_NUM + bucket] = 0;
                    }
                }
                else{//set one bit 0
                    unsigned char next_c = patterns_in_each_bucket[bucket][i][pattern_length - 1 - toleft_offset + 1];
                    int index = (c << SUPER_BIT_NUM) + next_c % (1 << SUPER_BIT_NUM);
                    //shiftorMasks[index][(MAX_PATTERN_LENGTH - toleft_offset - 1) * BUCKET_NUM + (BUCKET_NUM - bucket)] = 1;
                    shiftorMasks[index][toleft_offset * BUCKET_NUM + bucket] = 0;
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
    else{
        printf("wrong grouping method!\n");
        exit(-1);
    }

    //generate shift-or masks for each character
    generate_shiftor_mask();
}

void soengine::debug() {
    FILE* f = fopen("./shiftor_masks.txt", "w");
    for(int c=0; c < (1 << (8 + SUPER_BIT_NUM)); c++){
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

        //case_num[depth][char] 表示深度为depth时末尾字符为c的可能路径数。depth(从左往右数)
        uint64_t **case_num = (uint64_t **) malloc(sizeof(uint64_t *) * pattern_length);
        for(int depth = 0; depth < pattern_length; depth++){
            case_num[depth] = (uint64_t *) malloc(sizeof(uint64_t) * 256);
            for(int c = 0; c < 256; c++) case_num[depth][c] = 0;
        }

        for(int depth = 0; depth < pattern_length; depth++){
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


#define MATCH_DEBUG 0
void soengine::match(string *T) {
    /*初始化state-mask*/
    bitset<2 * MAX_PATTERN_LENGTH * BUCKET_NUM> state_mask;
    state_mask.reset(); //state-mask 所有位置为0
    //每一个bucket，小于pattern-length的位置置为1，避免误匹配
    for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
        if(patterns_in_each_bucket[bucket].size() < 1) continue;
        int pattern_length = patterns_in_each_bucket[bucket][0].size();
        for(int length = 0; length < pattern_length - 1; length++){
            //state_mask[(2 * MAX_PATTERN_LENGTH - length) * BUCKET_NUM - bucket - 1] = 1;
            state_mask[length * BUCKET_NUM + bucket] = 1;
        }
    }
    if(MATCH_DEBUG) std::cout << "stt_mask: " << state_mask.to_string() << endl;

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
            int index =  c1 * (1 << SUPER_BIT_NUM) + c2 % (1 << SUPER_BIT_NUM);
            so_masks[i].reset(); //初始化为全0
            //so_masks[i] = shiftorMasks[index];
            for(int length = 0; length < MAX_PATTERN_LENGTH; length++){
                for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
                    so_masks[i][length * BUCKET_NUM + bucket] = shiftorMasks[index][length * BUCKET_NUM + bucket];
                }
            }
            so_masks[i] = so_masks[i] << (i * BUCKET_NUM);

            if(MATCH_DEBUG) std::cout << "so_mask" << i << ": " << so_masks[i].to_string() << endl;
        }

        //shift-or运算
        for(int i = 0; i < MAX_PATTERN_LENGTH; i++){
            state_mask |= so_masks[i];
        }
        if(MATCH_DEBUG) std::cout << "state_mask: " << state_mask.to_string() << endl;

        //检测是否存在匹配:小于MAX_PATTERN_LENGTH的位置是否存在0 bit
        for(int length = 0; length < MAX_PATTERN_LENGTH; length++){
            bool match_flag = false;
            for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
                if(state_mask[length * BUCKET_NUM + bucket] == 0){
                    if(MATCH_DEBUG) printf("position %d match patterns in bucket %d\n", pos + length, bucket);
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
        int index =  c1 * (1 << SUPER_BIT_NUM) + c2 % (1 << SUPER_BIT_NUM);
        so_masks[i].reset(); //初始化为全0
        //so_masks[i] = shiftorMasks[index];
        for(int length = 0; length < MAX_PATTERN_LENGTH; length++){
            for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
                so_masks[i][length * BUCKET_NUM + bucket] = shiftorMasks[index][length * BUCKET_NUM + bucket];
            }
        }
        so_masks[i] = so_masks[i] << (i * BUCKET_NUM);

        if(MATCH_DEBUG) std::cout << "so_mask" << i << ": " << so_masks[i].to_string() << endl;
    }

    //shift-or运算
    for(int i = 0; i < left_length; i++){
        state_mask |= so_masks[i];
    }
    if(MATCH_DEBUG) std::cout << "stt_mask: " << state_mask.to_string() << endl;

    //检测是否存在匹配:小于MAX_PATTERN_LENGTH的位置是否存在0 bit
    for(int length = 0; length < left_length; length++){
        for(int bucket = 0; bucket < BUCKET_NUM; bucket++){
            if(state_mask[length * BUCKET_NUM + bucket] == 0){
                if(MATCH_DEBUG) printf("position %d match patterns in bucket %d\n", pos + length, bucket);
            }
        }
    }

    //report
    printf("match_num:%d\nmatch_num_each_length:%d\n", match_num, match_num_each_length);
}
