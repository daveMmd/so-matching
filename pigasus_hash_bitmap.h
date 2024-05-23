//
// Created by sczho on 2024/5/22.
//
#include <cstdint>
#include <vector>
#include <string>

#ifndef SO_MATCHING_PIGASUS_HASH_BITMAP_H
#define SO_MATCHING_PIGASUS_HASH_BITMAP_H
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
        int LSB_BYTES_MASKED;

        Hashtable_bitmap(int _NBITS, int _DWIDTH, int _MEM_SIZE, int _LSB_BYTES_MASKED){
            NBITS = _NBITS;
            DWIDTH = _DWIDTH;
            MEM_SIZE = _MEM_SIZE;
            LSB_BYTES_MASKED = _LSB_BYTES_MASKED;
            BM_AWIDTH = NBITS - 3;

            rom = new uint8_t[MEM_SIZE];
        }

        Hashtable_bimap_res lookup(uint32_t addr0, uint32_t addr1);

        bool lookup(std::string txt);

        void build(const std::vector<std::string>& patterns);

        uint32_t dave_hash(std::string txt); //code by reverse analysis

        void output_mif(std::string fname);
    };
}


#endif //SO_MATCHING_PIGASUS_HASH_BITMAP_H
