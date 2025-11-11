/***************************************************************/
/*                                                             */
/*   ARM Instruction Level Simulator                           */
/*                                                             */
/*   CMSC-22200 Computer Architecture                          */
/*   University of Chicago                                     */
/*                                                             */
/***************************************************************/

#ifndef _BP_H_
#define _BP_H_

#include <stdint.h>

struct Pipe_Op;

typedef struct
{
    /* gshare */
    int ghr_bits;
    uint32_t ghr;
    uint8_t *pht; /* size 2^ghr_bits */

    /* BTB */
    int btb_size;
    int btb_bits;
    uint64_t *btb_tag;
    uint64_t *btb_dest;
    uint8_t *btb_valid;
    uint8_t *btb_cond;
} bp_t;
extern bp_t bp;

void bp_t_init();

void bp_predict(struct Pipe_Op *op);
// In fetch:
    // we XOR current GHR with PC [9:2] to get PHT index --> store this value in Pipe_Op.GHR_XOR_PC
    // we index PHT to get 2-bit counter, if most significant bit is 1 (aka if 10 or 11 --> if 2 or 3), predict taken
    // we index BTB with PC[11:2] to get entry
    //if (valid bit is 1, address_tag == PC) && PHT says we should take branch, update PC to target address in BTB
    // else, predict not taken (aka PC + 4)
void bp_update(struct Pipe_Op *op);
// In execute after we have calculated whether the branch was actually taken:
    // IF unconditional, do not update PHT and GHR, else:
    // we update PHT by +- 1 based on whether branch was taken (use EX_to_MEM_CURRENT.GHR_XOR_PC to index)
    // we update GHR by shifting left 1 and +- 1 based on whether branch was taken
    // Update BTB as follows:
        // index BTB with PC[11:2]
        // if hit (which means that valid and indexed):
            // if address == PC and valid, then we update target to BR_TARGET
            // if address != PC, we update address_tag to PC, target to BR_TARGET, valid to 1, cond bit based on instruction type
        // else (miss, which means that row was not valid):
            // update address_tag to PC, target to BR_TARGET, valid to 1, cond bit based on instruction type
        // NOTE: for unconditional branches, all we have to do is:
            // if hit, set uncondional bit, and set target to BR_TARGET
            // if miss, set address_tag to PC, target to BR_TARGET, valid to 1, conditional bit to 0
        //not that was always update BR_TARGET regardless of whether we actually took it


#endif
