//
// Created by sczho on 2024/5/22.
//

#include <cassert>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "pigasus_hash_bitmap.h"
namespace Pigasus{

Mulhash_res mul_hash(uint8_t a){
    uint64_t b = 0x0b4e0ef37bc32127;

    uint16_t bn[4];
    uint64_t tmpb = b;
    for(int i = 0; i < 4; i++){
        bn[i] = tmpb ;
        tmpb = (tmpb >> 16);
    }

    Mulhash_res res;
    for(int i = 0; i < 4; i++){
        res.ab_n[i] = a * bn[i];
    }

    return res;
}

int countTrailingZeroBytes(uint64_t mask) {
    int count = 0;

    // 每次检查最低的8位（一个字节）
    while ((mask & 0xFF) == 0 && count < 8) {
        count++;
        mask >>= 8; // 右移8位（一个字节）
    }

    return count;
}

uint32_t acc_hash(uint64_t ANDMSK, int NBITS, std::vector<std::vector<uint32_t>> ai_bj_vec){
    assert(ai_bj_vec[0].size() == 4);
    assert(ai_bj_vec[1].size() == 4);
    assert(ai_bj_vec[2].size() == 3);
    assert(ai_bj_vec[3].size() == 3);
    assert(ai_bj_vec[4].size() == 2);
    assert(ai_bj_vec[5].size() == 2);
    assert(ai_bj_vec[6].size() == 1);
    assert(ai_bj_vec[7].size() == 1);

    int LSB_BYTES_MASKED = countTrailingZeroBytes(ANDMSK);

    auto msk_ai_bj_vec = ai_bj_vec;
    if(LSB_BYTES_MASKED >= 1){
        for(int j = 0; j < 4; j++)
            msk_ai_bj_vec[0][j] = 0;
    }
    if(LSB_BYTES_MASKED >= 2){
        for(int j = 0; j < 4; j++)
            msk_ai_bj_vec[1][j] = 0;
    }
    if(LSB_BYTES_MASKED >= 3){
        for(int j = 0; j < 3; j++)
            msk_ai_bj_vec[2][j] = 0;
    }
    if(LSB_BYTES_MASKED >= 4){
        for(int j = 0; j < 3; j++)
            msk_ai_bj_vec[3][j] = 0;
    }
    if(LSB_BYTES_MASKED >= 5){
        for(int j = 0; j < 2; j++)
            msk_ai_bj_vec[4][j] = 0;
    }
    if(LSB_BYTES_MASKED >= 6){
        for(int j = 0; j < 2; j++)
            msk_ai_bj_vec[5][j] = 0;
    }
    if(LSB_BYTES_MASKED >= 7){
        msk_ai_bj_vec[6][0] = 0;
    }
    if(LSB_BYTES_MASKED == 8){
        msk_ai_bj_vec[7][0] = 0;
    }

    //level1
    uint64_t a01_b0 = msk_ai_bj_vec[0][0] + (msk_ai_bj_vec[1][0] << 8);//msk_a0b0 + {msk_a1b0, 8'd0};
    uint64_t a23_b0 = msk_ai_bj_vec[2][0] + (msk_ai_bj_vec[3][0] << 8);//msk_a2b0 + {msk_a3b0, 8'd0};
    uint64_t a45_b0 = msk_ai_bj_vec[4][0] + (msk_ai_bj_vec[5][0] << 8);//msk_a4b0 + {msk_a5b0, 8'd0};
    uint64_t a67_b0 = msk_ai_bj_vec[6][0] + (msk_ai_bj_vec[7][0] << 8);//msk_a6b0 + {msk_a7b0, 8'd0};
    uint64_t a01_b1 = msk_ai_bj_vec[0][1] + (msk_ai_bj_vec[1][1] << 8);//msk_a0b1 + {msk_a1b1, 8'd0};
    uint64_t a23_b1 = msk_ai_bj_vec[2][1] + (msk_ai_bj_vec[3][1] << 8);//msk_a2b1 + {msk_a3b1, 8'd0};
    uint64_t a45_b1 = msk_ai_bj_vec[4][1] + (msk_ai_bj_vec[5][1] << 8);//msk_a4b1 + {msk_a5b1, 8'd0};
    uint64_t a01_b2 = msk_ai_bj_vec[0][2] + (msk_ai_bj_vec[1][2] << 8);//msk_a0b2 + {msk_a1b2, 8'd0};
    uint64_t a23_b2 = msk_ai_bj_vec[2][2] + (msk_ai_bj_vec[3][2] << 8);//msk_a2b2 + {msk_a3b2, 8'd0};
    uint64_t a01_b3 = msk_ai_bj_vec[0][3] + (msk_ai_bj_vec[1][3] << 8);//msk_a0b3 + {msk_a1b3, 8'd0};

    //level2
    /*
    add_a01_b1_a23_b0 <= a01_b1       + a23_b0;
    add_a01_b2_a45_b0 <= a01_b2       + a45_b0;
    add_a01_b3_a23_b2 <= a01_b3[15:0] + a23_b2[15:0];
    add_a45_b1_a67_b0 <= a45_b1[15:0] + a67_b0[15:0];
    a01_b0_reg        <= a01_b0;
    a23_b1_reg        <= a23_b1;*/
    uint64_t add_a01_b1_a23_b0 = a01_b1     +   a23_b0;
    uint64_t add_a01_b2_a45_b0 = a01_b2     +   a45_b0;
    uint64_t add_a01_b3_a23_b2 = (a01_b3 % (1 << 16)) + (a23_b2 % (1 << 16));
    uint64_t add_a45_b1_a67_b0 = (a45_b1 % (1 << 16)) + (a67_b0 % (1 << 16));

    //level3
    uint64_t sum0 = (add_a01_b1_a23_b0<<16)  + a01_b0;
    uint64_t sum1 =  add_a01_b2_a45_b0       + a23_b1;
    uint64_t sum2 =  (add_a01_b3_a23_b2 % (1 << 16)) + (add_a45_b1_a67_b0 % (1 << 16));

    // Level 4
    uint64_t half_sum0 = sum0;
    uint64_t half_sum1 = ((sum2 % (1 << 16)) << 16) + sum1;

    //Final Addition

    uint64_t sum = half_sum0 + ((half_sum1 & 0xffffffff) << 32);

    uint32_t p = (sum >> (64 - NBITS));

    return p;
}

    Hashtable_bimap_res Hashtable_bitmap::lookup(uint32_t addr0, uint32_t addr1) {
        Hashtable_bimap_res res;
        uint32_t bm_addr0 = (addr0 >> 3) % (1 << (NBITS - 3));
        uint32_t bm_addr1 = (addr1 >> 3) % (1 << (NBITS - 3));
        uint8_t  bm_bit0 = addr0 % (1 << 3);
        uint8_t  bm_bit1 = addr1 % (1 << 3);

        uint8_t q0 = rom[bm_addr0];
        uint8_t q1 = rom[bm_addr1];

        //propogate addresses
        res.dout0 = addr0;
        res.dout1 = addr1;

        res.dout0_valid = ((q0 >> bm_bit0) & 1);
        res.dout1_valid = ((q1 >> bm_bit1) & 1);

        return res;
    }

    uint32_t Hashtable_bitmap::dave_hash(std::string txt){
        //padding 0 from leftmost
        std::string paddedString = std::string(8 - txt.length(), '0') + txt;

        assert(paddedString.size() == 8);

        Mulhash_res aibj[8];

        for(int i = 0; i < 8; i++){
            aibj[i] = mul_hash(paddedString[i]);
        }

        //mask leftmost bytes
        for(int i = 0; i < LSB_BYTES_MASKED; i++){
            for(int j = 0; j < 4; j++) aibj[i].ab_n[j] = 0;
        }

        //level1
        uint64_t a01_b0 = aibj[0].ab_n[0] + (aibj[1].ab_n[0] << 8);
        uint64_t a23_b0 = aibj[2].ab_n[0] + (aibj[3].ab_n[0] << 8);
        uint64_t a45_b0 = aibj[4].ab_n[0] + (aibj[5].ab_n[0] << 8);
        uint64_t a67_b0 = aibj[6].ab_n[0] + (aibj[7].ab_n[0] << 8);
        uint64_t a01_b1 = aibj[0].ab_n[1] + (aibj[1].ab_n[1] << 8);
        uint64_t a23_b1 = aibj[2].ab_n[1] + (aibj[3].ab_n[1] << 8);
        uint64_t a45_b1 = aibj[4].ab_n[1] + (aibj[5].ab_n[1] << 8);
        uint64_t a01_b2 = aibj[0].ab_n[2] + (aibj[1].ab_n[2] << 8);
        uint64_t a23_b2 = aibj[2].ab_n[2] + (aibj[3].ab_n[2] << 8);
        uint64_t a01_b3 = aibj[0].ab_n[3] + (aibj[1].ab_n[3] << 8);

        //level2
        uint64_t add_a01_b1_a23_b0 = a01_b1 + a23_b0;
        uint64_t add_a01_b2_a45_b0 = a01_b2 + a45_b0;
        uint64_t add_a01_b3_a23_b2 = (a01_b3 % 65536) + (a23_b2 % 65536);
        uint64_t add_a45_b1_a67_b0 = (a45_b1 % 65536) + (a67_b0 % 65536);

        //level3
        uint64_t sum0 = (add_a01_b1_a23_b0 << 16) + a01_b0;
        uint64_t sum1 = add_a01_b2_a45_b0         + a23_b1;
        uint64_t sum2 = (add_a01_b3_a23_b2 % 65536) + (add_a45_b1_a67_b0 % 65536);

        //level4
        uint64_t half_sum0 = sum0;
        uint64_t half_sum1 = (sum2 << 16) + sum1;

        //Final Addition
        uint64_t sum = half_sum0 + (half_sum1 << 32);

        uint32_t addr = (sum >> (64 - NBITS));

        return addr;
}
    void Hashtable_bitmap::build(const std::vector<std::string>& patterns) {
        for(int i = 0; i < MEM_SIZE; i++) rom[i] = 0;

        if(patterns.empty()) return;

        for(auto pattern : patterns){
            //calculate aibj / msk_aibj
            auto addr = dave_hash(pattern);
            //record rule addr
            if(fastPatternToRuleIDMap != nullptr){ //only FSPM record
                if(hashtable_addr2ruleID[addr] != INVALID_RULEID){
                    fprintf(stderr, "hashtable_addr2ruleID conflicts!\n");
                    exit(-1);
                }
                hashtable_addr2ruleID[addr] = findRuleID(pattern);
            }

            uint32_t rom_addr = (addr >> 3);
            uint8_t  bm_addr = addr % (1 << 3);

            uint8_t tmp_bitmap = rom[rom_addr];
            //rom[rom_addr] = tmp_bitmap | (1 << (7 - bm_addr));
            rom[rom_addr] = tmp_bitmap | (1 << bm_addr);
            //debug
            auto read_bm = rom[rom_addr];
        }

    }

    bool Hashtable_bitmap::lookup(std::string txt) {
        auto addr = dave_hash(txt);

        uint32_t bm_addr = (addr >> 3) % (1 << (NBITS - 3));
        uint8_t  bm_bit = addr % (1 << 3);

        uint8_t q0 = rom[bm_addr];

        bool if_match = ((q0 >> bm_bit) & 1);

        if(if_match){
            printf("hash_out (hash_addr): %x\n", addr); //hash_out in Pigasus (Used for Rule ID?)
        }


        return if_match;
    }

    void Hashtable_bitmap::output_mif(std::string fname_bitmap, std::string fname_hashtable) {
        bitmap_mif_write(fname_bitmap);

        hashtable_mif_write(fname_hashtable);
    }

    uint16_t Hashtable_bitmap::findRuleID(std::string pattern) {
        //NFPSM
        if(fastPatternToRuleIDMap == nullptr) return 0;

        auto it = fastPatternToRuleIDMap->find(pattern);
        if(it == fastPatternToRuleIDMap->end()){
            //return 0; //NFPM no need to record ruleid...
            fprintf(stderr, "error: not found pattern to sid!\n");
            exit(-1);
        }
        return (*it).second;
    }

    void Hashtable_bitmap::bitmap_mif_write(std::string fname) {
        std::ofstream mifFile(fname);
        if (!mifFile.is_open()) {
            std::cerr << "Failed to open file: " << fname << std::endl;
            return;
        }

        // 写入 MIF 文件头
        mifFile << "WIDTH=8;\n";
        mifFile << "DEPTH=" << MEM_SIZE << ";\n\n";
        mifFile << "ADDRESS_RADIX=HEX;\n";
        mifFile << "DATA_RADIX=HEX;\n\n";
        mifFile << "CONTENT BEGIN\n";

        // 写入数组内容
        for (size_t i = 0; i < MEM_SIZE; ++i) {
            mifFile << "    " << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << i << " : "
                    << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(rom[i]) << ";\n";
        }

        mifFile << "END;\n";
        mifFile.close();
        std::cout << "MIF file written successfully to " << fname << std::endl;
    }

    void Hashtable_bitmap::hashtable_mif_write(std::string fname) {
        std::ofstream mifFile(fname);
        if (!mifFile.is_open()) {
            std::cerr << "Failed to open file: " << fname << std::endl;
            return;
        }

        // 写入 MIF 文件头
        mifFile << "WIDTH=16;\n";
        mifFile << "DEPTH=" << addr2rule_map_size << ";\n\n";
        mifFile << "ADDRESS_RADIX=HEX;\n";
        mifFile << "DATA_RADIX=HEX;\n\n";
        mifFile << "CONTENT BEGIN\n";

        // 写入数组内容
        for (size_t i = 0; i < addr2rule_map_size; ++i) {
            mifFile << "    " << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << i << " : "
                    << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << static_cast<int>(hashtable_addr2ruleID[i]) << ";\n";
        }

        mifFile << "END;\n";
        mifFile.close();
        std::cout << "MIF file written successfully to " << fname << std::endl;
    }
}