/***************************************************************/
/*                                                             */
/*   ARM Instruction Level Simulator                           */
/*                                                             */
/*   CMSC-22200 Computer Architecture                          */
/*   University of Chicago                                     */
/*                                                             */
/***************************************************************/

#include "bp.h"
#include <stdlib.h>
#include <stdio.h>
#include "pipe.h"
#include <string.h>

bp_t bp;

static uint32_t bp_extract_bits(uint32_t instruction, int start, int end){
    /* Given an instruction type, returns a section from start: end (inclusive)*/

    int width = end - start + 1;
    uint32_t mask = (1U << width) - 1;
    return (instruction >> start) & mask;
}

void bp_t_init()
{
    memset(&bp, 0, sizeof(bp_t)); 
    bp.ghr_bits = 8;
    bp.ghr = 0;

    bp.pht = (uint8_t*)calloc(256, sizeof(uint8_t));
    bp.btb_size = 1024;
    bp.btb_bits = 10;
    bp.btb_tag = (uint64_t*)calloc(bp.btb_size, sizeof(uint64_t));
    bp.btb_dest = (uint64_t*)calloc(bp.btb_size, sizeof(uint64_t));
    bp.btb_valid = (uint8_t*)calloc(bp.btb_size, sizeof(uint8_t));
    bp.btb_cond = (uint8_t*)calloc(bp.btb_size, sizeof(uint8_t)); 

}

void bp_predict(struct Pipe_Op *op)
{
    uint64_t pc = op -> PC;
    unsigned int pht_index = bp_extract_bits(pc, 2, 9) ^ bp.ghr;
    op -> GHR_XOR_PC = pht_index;
    uint8_t counter = bp.pht[pht_index];
    unsigned int btb_index = bp_extract_bits(pc, 2, 11);
    if (bp.btb_valid[btb_index] && (bp.btb_tag[btb_index] == pc)) {
        op -> BTB_MISS = 0;
        if (counter >= 2) {
            op -> PREDICTED_PC = bp.btb_dest[btb_index];
        } else if (bp.btb_cond[btb_index] == 0) {
            op -> PREDICTED_PC = bp.btb_dest[btb_index];
        } else {
            op -> PREDICTED_PC = pc + 4;
        }
    } else {
        op -> BTB_MISS = 1;
        op -> PREDICTED_PC = pc + 4;
    }
}


void bp_update(struct Pipe_Op *op)
{
    if (op -> CBRANCH) {
        bp.pht[op -> GHR_XOR_PC] = 
            (op->BR_TAKEN && bp.pht[op -> GHR_XOR_PC] < 3) ? bp.pht[op -> GHR_XOR_PC] + 1 :
            (!op->BR_TAKEN && bp.pht[op -> GHR_XOR_PC] > 0) ? bp.pht[op -> GHR_XOR_PC] - 1 :
            bp.pht[op -> GHR_XOR_PC];
        bp.ghr = (bp.ghr << 1) + op->BR_TAKEN;
    }
    unsigned int btb_index = bp_extract_bits(op -> PC, 2, 11);
    if (bp.btb_valid[btb_index]) {
        if (bp.btb_tag[btb_index] == op -> PC) {
            bp.btb_dest[btb_index] = op -> BR_TARGET;
        } else {
            bp.btb_tag[btb_index] = op -> PC;
            bp.btb_dest[btb_index] = op -> BR_TARGET;
            bp.btb_valid[btb_index] = 1;
            bp.btb_cond[btb_index] = op -> CBRANCH ? 1 : 0;
        }
    } else {
        bp.btb_tag[btb_index] = op -> PC;
        bp.btb_dest[btb_index] = op -> BR_TARGET;
        bp.btb_valid[btb_index] = 1;
        bp.btb_cond[btb_index] = op -> CBRANCH ? 1 : 0;
    }
}
