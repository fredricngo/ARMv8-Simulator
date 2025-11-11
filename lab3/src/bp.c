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

static uint32_t bp_extract_bits(uint64_t instruction, int start, int end){
    /* Given an instruction type, returns a section from start: end (inclusive)*/

    int width = end - start + 1;
    uint64_t mask = (1U << width) - 1;
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

// void bp_predict(struct Pipe_Op *op)
// {
//     uint64_t pc = op -> PC;
//     unsigned int pht_index = bp_extract_bits(pc, 2, 9) ^ bp.ghr;
//     op -> GHR_XOR_PC = pht_index;
//     uint8_t counter = bp.pht[pht_index];
//     unsigned int btb_index = bp_extract_bits(pc, 2, 11);
//     if (bp.btb_valid[btb_index] && (bp.btb_tag[btb_index] == pc)) {
//         op -> BTB_MISS = 0;
//         if (counter >= 2) {
//             op -> PREDICTED_PC = bp.btb_dest[btb_index];
//         } else if (bp.btb_cond[btb_index] == 0) {
//             op -> PREDICTED_PC = bp.btb_dest[btb_index];
//         } else {
//             op -> PREDICTED_PC = pc + 4;
//         }
//     } else {
//         op -> BTB_MISS = 1;
//         op -> PREDICTED_PC = pc + 4;
//     }
// }
void bp_predict(struct Pipe_Op *op)
{
    uint64_t pc = op->PC;
    uint32_t ghr_mask = (1u << bp.ghr_bits) - 1;
    unsigned int pht_index = (bp_extract_bits(pc, 2, 9) ^ bp.ghr) & ghr_mask;
    op->GHR_XOR_PC = pht_index;
    uint8_t counter = bp.pht[pht_index];
    unsigned int btb_index = bp_extract_bits(pc, 2, 11);
    
    printf("BP_PREDICT: PC=0x%lx, GHR=0x%x, PHT_idx=%d, counter=%d, BTB_idx=%d\n", 
           pc, bp.ghr, pht_index, counter, btb_index);
    
    if (bp.btb_valid[btb_index] && (bp.btb_tag[btb_index] == pc)) {
        op->BTB_MISS = 0;
        printf("BTB_HIT: tag=0x%lx, dest=0x%lx, cond=%d\n", 
               bp.btb_tag[btb_index], bp.btb_dest[btb_index], bp.btb_cond[btb_index]);
        
        if (counter >= 2) {
            op->PREDICTED_PC = bp.btb_dest[btb_index];
            printf("PREDICT_TAKEN: counter=%d, predicted_pc=0x%lx\n", counter, op->PREDICTED_PC);
        } else if (bp.btb_cond[btb_index] == 0) {
            op->PREDICTED_PC = bp.btb_dest[btb_index];
            printf("UNCOND_BRANCH: predicted_pc=0x%lx\n", op->PREDICTED_PC);
        } else {
            op->PREDICTED_PC = pc + 4;
            printf("PREDICT_NOT_TAKEN: counter=%d, predicted_pc=0x%lx\n", counter, op->PREDICTED_PC);
        }
    } else {
        op->BTB_MISS = 1;
        op->PREDICTED_PC = pc + 4;
        printf("BTB_MISS: predicted_pc=0x%lx\n", op->PREDICTED_PC);
    }
}

// void bp_update(struct Pipe_Op *op)
// {
//     if (op -> CBRANCH) {
//         bp.pht[op -> GHR_XOR_PC] = 
//             (op->BR_TAKEN && bp.pht[op -> GHR_XOR_PC] < 3) ? bp.pht[op -> GHR_XOR_PC] + 1 :
//             (!op->BR_TAKEN && bp.pht[op -> GHR_XOR_PC] > 0) ? bp.pht[op -> GHR_XOR_PC] - 1 :
//             bp.pht[op -> GHR_XOR_PC];
//         bp.ghr = (bp.ghr << 1) + op->BR_TAKEN;
//     }
//     unsigned int btb_index = bp_extract_bits(op -> PC, 2, 11);
void bp_update(struct Pipe_Op *op)
{
    printf("BP_UPDATE: PC=0x%lx, BR_TAKEN=%d, BR_TARGET=0x%lx, PREDICTED_PC=0x%lx\n",
           op->PC, op->BR_TAKEN, op->BR_TARGET, op->PREDICTED_PC);
    
    if (op->CBRANCH) {
        uint8_t old_counter = bp.pht[op->GHR_XOR_PC];
        bp.pht[op->GHR_XOR_PC] = 
            (op->BR_TAKEN && bp.pht[op->GHR_XOR_PC] < 3) ? bp.pht[op->GHR_XOR_PC] + 1 :
            (!op->BR_TAKEN && bp.pht[op->GHR_XOR_PC] > 0) ? bp.pht[op->GHR_XOR_PC] - 1 :
            bp.pht[op->GHR_XOR_PC];
        
        printf("PHT_UPDATE: idx=%d, old_counter=%d, new_counter=%d\n",
               op->GHR_XOR_PC, old_counter, bp.pht[op->GHR_XOR_PC]);
        
        uint8_t old_ghr = bp.ghr;
        uint32_t ghr_mask = (1u << bp.ghr_bits) - 1;
        bp.ghr = ((bp.ghr << 1) | op->BR_TAKEN) & ghr_mask;
        printf("GHR_UPDATE: old=0x%x, new=0x%x\n", old_ghr, bp.ghr);
    }
    
    unsigned int btb_index = bp_extract_bits(op->PC, 2, 11);
    printf("BTB_UPDATE: idx=%d, PC=0x%lx, target=0x%lx, cond=%d\n",
           btb_index, op->PC, op->BR_TARGET, op->CBRANCH ? 1 : 0);
    if (bp.btb_valid[btb_index]) {
        if (bp.btb_tag[btb_index] == op -> PC) {
            bp.btb_dest[btb_index] = op -> BR_TARGET;
            bp.btb_cond[btb_index] = op -> CBRANCH ? 1 : 0;
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
