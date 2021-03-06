//
// Created by 钟金诚 on 2021/3/1.
//

#ifndef SO_MATCHING_SOENGINE_H
#define SO_MATCHING_SOENGINE_H

#include <cstdint>
#include <list>
#include <string>
#include <bitset>
#include <vector>
using namespace std;

/*DEBUG FLAG*/
#define DEBUG_MATCHING_CASES 0
#define PATTERN_GROUPING 1 //2：局部搜索划分；1:用hypersan的动态规划方法划分规则集； 0:平均划分；

//#define SUPER_BIT_NUM 4
#define SUPER_BIT_NUM 2
//#define BUCKET_NUM 8
#define BUCKET_NUM 32
#define MAX_PATTERN_LENGTH 8
#define CUT_DIRECTION 1 //1为反向，0为正向

#define MAX_PATTERN_NUM 100005
typedef bitset<MAX_PATTERN_LENGTH * BUCKET_NUM> shiftor_mask_t;
typedef bitset<2 * MAX_PATTERN_LENGTH * BUCKET_NUM> so_mask_t;//用于匹配过程中

class soengine {
    //shift-or mask for each character
    shiftor_mask_t shiftorMasks[1 << (8 + SUPER_BIT_NUM)];
    vector<string> patterns_in_each_bucket[BUCKET_NUM];

public:
    explicit soengine(list<string>*, int groping_method = 0);

    void generate_shiftor_mask();

    /*匹配输入，未考虑性能优化*/
    void match(string *);

    void debug();

    void show_duplicate_patterns_in_same_bucket();

    /*计算过滤流量比例（假设输入流量完全随机）*/
    double filtering_effectiveness();
};


#endif //SO_MATCHING_SOENGINE_H
