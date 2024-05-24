//
// Created by sczho on 2024/5/22.
//
#include <cstdint>
#include <vector>
#include <string>
#include <map>

#ifndef SO_MATCHING_PIGASUS_HASH_BITMAP_H
#define SO_MATCHING_PIGASUS_HASH_BITMAP_H

#define INVALID_RULEID 0xffff
namespace Pigasus{
    typedef struct {
        uint32_t ab_n[4]; //[23:0]
    }Mulhash_res;

    Mulhash_res mul_hash(uint8_t a);

    uint32_t acc_hash(std::vector<std::vector<uint32_t>> ai_bj_vec);

    typedef struct {
        uint32_t dout0; //propogate addresses
        uint32_t dout1; //propogate addresses
        bool dout0_valid; //is pos 0 valid (really match)
        bool dout1_valid; //is pos 1 valid (really match)
    }Hashtable_bimap_res;

    class Hashtable_bitmap{
    public:
        int NBITS;
        int DWIDTH;
        int MEM_SIZE;
        int BM_AWIDTH;
        uint8_t *rom;
        uint16_t *hashtable_addr2ruleID;
        int addr2rule_map_size;
        int LSB_BYTES_MASKED;

        std::map<std::string, uint16_t > *fastPatternToRuleIDMap;

        Hashtable_bitmap(int _NBITS, int _DWIDTH, int _LSB_BYTES_MASKED, int _addr2rule_map_size){
            NBITS = _NBITS;
            DWIDTH = _DWIDTH;
            MEM_SIZE = (1 << (NBITS - 3));
            LSB_BYTES_MASKED = _LSB_BYTES_MASKED;
            BM_AWIDTH = NBITS - 3;

            rom = new uint8_t[MEM_SIZE];

            addr2rule_map_size = _addr2rule_map_size;
            hashtable_addr2ruleID = new uint16_t[addr2rule_map_size];
            for(int i = 0; i < addr2rule_map_size; i++)
                hashtable_addr2ruleID[i] = INVALID_RULEID;
        }

        Hashtable_bimap_res lookup(uint32_t addr0, uint32_t addr1);

        bool lookup(std::string txt);

        void build(const std::vector<std::string>& patterns);

        uint16_t findRuleID(std::string pattern);

        uint32_t dave_hash(std::string txt); //code by reverse analysis

        void output_mif(std::string fname_bitmap, std::string fname_hashtable);

        void bitmap_mif_write(std::string fname);

        void hashtable_mif_write(std::string fname);
    };
}


#endif //SO_MATCHING_PIGASUS_HASH_BITMAP_H
