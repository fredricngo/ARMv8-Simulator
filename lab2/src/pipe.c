/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 */

#include "pipe.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* global pipeline state */
Pipe_State pipe;
Pipe_Op IF_to_DE_CURRENT;
Pipe_Op DE_to_EX_CURRENT;
Pipe_Op EX_to_MEM_CURRENT;
Pipe_Op MEM_to_WB_CURRENT;

Pipe_Op IF_to_DE_PREV;
Pipe_Op DE_to_EX_PREV;
Pipe_Op EX_to_MEM_PREV;
Pipe_Op MEM_to_WB_PREV;

//STALLING LOGIC
Pipe_Op SAVED_INSTRUCTION;
int INSTRUCTION_SAVED = 0;
uint64_t REFETCH_ADDR  = 0;
int REFETCH_PC = 0;
int REFETCH_PC_NEXT = 0;
int UPDATE_EX = 0;
int UPDATE_EX_NEXT = 0;

int HLT_FLAG = 0;
int HLT_NEXT = 0;
int RUN_BIT;

int CLEAR_DE = 0;
int SQUASH = 0;
int BRANCH = 0;
int BRANCH_NEXT = 0;


int64_t bit_extension(int32_t immediate, int start, int end){
    /*
    This function receives an immediate (int32_t) and: 
    (1) Checks if the MSB is set to 1 (signed int)
    (2) if (1), extends to a uint64_t for arithmetic.
    */

    int width = end - start + 1;
    if ((immediate >> (width - 1)) & 1) {
        uint64_t mask = ~((1ULL << width) - 1); 
        return (int64_t) (immediate | mask);
    } else {
        return (int64_t) immediate; 
    }
}

uint32_t extract_bits(uint32_t instruction, int start, int end){
    /* Given an instruction type, returns a section from start: end (inclusive)*/

    int width = end - start + 1;
    uint32_t mask = (1U << width) - 1;
    return (instruction >> start) & mask;
}

////this can also be called for the taken conditional branch bc the logic is the same!
void unconditional_branching(void) {
    if (!(MEM_to_WB_CURRENT.BR_TARGET == IF_to_DE_CURRENT.PC)) {
        printf("BRANCH MISPREDICTION - Redirecting to 0x%lx\n", 
        MEM_to_WB_CURRENT.BR_TARGET);
        pipe.PC = MEM_to_WB_CURRENT.BR_TARGET;
        SQUASH = 1;
        stat_squash ++;
        CLEAR_DE = 1;
            }
}

void pipe_init()
{
    memset(&pipe, 0, sizeof(Pipe_State));
    pipe.PC = 0x00400000;
    RUN_BIT = TRUE;
    printf("Pipeline initialized: PC=0x%lx\n", pipe.PC);
}


void pipe_cycle()
{
	pipe_stage_wb();
	pipe_stage_mem();
	pipe_stage_execute();
	pipe_stage_decode();
    pipe_stage_fetch();

    REFETCH_PC = REFETCH_PC_NEXT;
    REFETCH_PC_NEXT = 0;
    UPDATE_EX = UPDATE_EX_NEXT;
    UPDATE_EX_NEXT = 0;
    BRANCH = BRANCH_NEXT;
    BRANCH_NEXT = 0;

    HLT_FLAG = HLT_NEXT;

    IF_to_DE_PREV = IF_to_DE_CURRENT;
    DE_to_EX_PREV = DE_to_EX_CURRENT;
    EX_to_MEM_PREV = EX_to_MEM_CURRENT;
    MEM_to_WB_PREV = MEM_to_WB_CURRENT;
}


void write_register(int reg_num, int64_t value){
    if (reg_num != 31){
        pipe.REGS[reg_num] = value; 
    }
}

int64_t read_register(int reg_num){
    if (reg_num == 31){
        return 0;
    }
    return pipe.REGS[reg_num];
}


void pipe_stage_wb()
{
    Pipe_Op in = MEM_to_WB_PREV;
    if (in.NOP) return;

    switch (in.INSTRUCTION) {
        case ADD_EXT:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break;
        case ADD_IMM:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break; 
        case ADDS_IMM:
            write_register(in.RD_REG, in.result);
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
            stat_inst_retire++;
            break; 
        case ADDS_EXT:
            write_register(in.RD_REG, in.result);
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
            stat_inst_retire++;
            break;
        case AND_SHIFTR:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break; 
        case ANDS_SHIFTR:
            write_register(in.RD_REG, in.result);
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
            stat_inst_retire++;
            break;
        case EOR_SHIFTR:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break;
        case LDURB:
            write_register(in.RT_REG, in.MEM_DATA);
            stat_inst_retire++;
            break;
        case LSL_IMM:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break;
        case LSR_IMM:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break;
        case MOVZ:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break;
        case MUL:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break;
        case SUB_IMM:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++; 
            break;
        case SUB_EXT:
            write_register(in.RD_REG, in.result); 
            stat_inst_retire++;
            break; 
        case SUBS_IMM:
            write_register(in.RD_REG, in.result);
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
            stat_inst_retire++;
            break;
        case SUBS_EXT:
            write_register(in.RD_REG, in.result);
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
            stat_inst_retire++;
            break;
        case HLT:
            RUN_BIT = 0;
            stat_inst_retire++;
            break;
        case STUR_32:
            stat_inst_retire++;
            break;
        case STUR_64:
            stat_inst_retire++;
            break;
        case STURB:
            stat_inst_retire++;
            break;
        case STURH:
            stat_inst_retire++;
            break; 
        case LDUR_32:
            write_register(in.RT_REG, in.MEM_DATA);
            stat_inst_retire++;
            break;
        case LDUR_64:
            write_register(in.RT_REG, in.MEM_DATA);
            stat_inst_retire++;
            break;
        case LDURH:
            write_register(in.RT_REG, in.MEM_DATA);
            stat_inst_retire++;
            break;
        case CMP_EXT:
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
            stat_inst_retire++;
            break;
        case CMP_IMM:
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
            stat_inst_retire++;
            break;
        case BR:
            stat_inst_retire++;
            break;
        case B:
            stat_inst_retire++;
            break;
        case BEQ:
            stat_inst_retire++;
            break;
        case BNE:
            stat_inst_retire++;
            break;
        case BLT:
            stat_inst_retire++;
            break;
        case BLE:
            stat_inst_retire++;
            break;
        case BGT:
            stat_inst_retire++;
            break;
        case BGE:
            stat_inst_retire++;
            break;
    }
}

void pipe_stage_mem()
{
    memset(&MEM_to_WB_CURRENT, 0, sizeof(MEM_to_WB_CURRENT));
    Pipe_Op in = EX_to_MEM_PREV;
    if (in.NOP) { MEM_to_WB_CURRENT.NOP = 1; return; }

    MEM_to_WB_CURRENT = in;

    switch (in.INSTRUCTION) {
        case ADD_EXT:
            break;
        case ADD_IMM:
            break;
        case ADDS_IMM:
            break; 
        case ADDS_EXT:
            break;
        case AND_SHIFTR:
            break; 
        case ANDS_SHIFTR:
            break;
        case EOR_SHIFTR:
            break; 
        case LDURB:
        {
            uint32_t word = mem_read_32(MEM_to_WB_CURRENT.MEM_ADDRESS & ~3);
            int byte_offset = MEM_to_WB_CURRENT.MEM_ADDRESS & 3;
            uint8_t byte_data = (word >> (byte_offset * 8)) & 0xFF;
            MEM_to_WB_CURRENT.MEM_DATA = (uint64_t) byte_data;
            break;
        }
        case LSL_IMM:
            break;
        case LSR_IMM:
            break;
        case MOVZ:
            break;
        case MUL:
            break; 
        case SUB_IMM:
            break; 
        case SUB_EXT:
            break; 
        case SUBS_IMM:
            break;
        case SUBS_EXT:
            break;
        case HLT:
            break;
        case STUR_32:
            mem_write_32(in.MEM_ADDRESS, (uint32_t)(in.RT_VAL & 0xFFFFFFFF));
            break;
        case STUR_64:
            mem_write_32(in.MEM_ADDRESS, (uint32_t)(in.RT_VAL & 0xFFFFFFFF));
            mem_write_32(in.MEM_ADDRESS + 4, (uint32_t)(in.RT_VAL >> 32));
            break;
        case STURB:
        {
            uint32_t word = mem_read_32(in.MEM_ADDRESS & ~3);
            int byte_offset = in.MEM_ADDRESS & 3;
            uint32_t mask = 0xFF << (byte_offset * 8);
            uint32_t new_word = (word & ~mask) | ((in.RT_VAL & 0xFF) << (byte_offset * 8));
            mem_write_32(in.MEM_ADDRESS & ~3, new_word);
            break;
        }
        case STURH:
            mem_write_32(in.MEM_ADDRESS, (uint32_t)(in.RT_VAL & 0xFFFF));
            break;
        case LDUR_32:
            MEM_to_WB_CURRENT.MEM_DATA = (int32_t) mem_read_32(in.MEM_ADDRESS);
            break;
        case LDUR_64:
        {
            uint32_t low_word = mem_read_32(in.MEM_ADDRESS);
            uint32_t high_word = mem_read_32(in.MEM_ADDRESS + 4);
            MEM_to_WB_CURRENT.MEM_DATA = ((uint64_t)high_word << 32) | low_word;
            printf("LDUR_64 DEBUG: Loading from addr 0x%lx, low=0x%x, high=0x%x, result=0x%lx\n",
            in.MEM_ADDRESS, low_word, high_word, MEM_to_WB_CURRENT.MEM_DATA);
            break;
        }
        case LDURH:
            MEM_to_WB_CURRENT.MEM_DATA = (int16_t) mem_read_32(in.MEM_ADDRESS);
            break;
        case CMP_EXT:
            break;
        case CMP_IMM:
            break;
        case BR:
            unconditional_branching();
            break;
        case B:
        //this is the same case as a taken conditional branch (squash case)
            unconditional_branching();
            break;
        case BEQ:
            if (MEM_to_WB_CURRENT.BR_TAKEN) {
                unconditional_branching();
            } else {
                printf("BEQ: Branch not taken, continuing sequentially.\n");
            }
            break;
        case BNE:
            if (MEM_to_WB_CURRENT.BR_TAKEN) {
                unconditional_branching();
            } else {
                printf("BNE: Branch not taken, continuing sequentially.\n");
    }        break;
        case BLT:
            if (MEM_to_WB_CURRENT.BR_TAKEN) {
                unconditional_branching();
            } else {
                printf("BLT: Branch not taken, continuing sequentially.\n");
            }
            break;
        case BLE:
            if (MEM_to_WB_CURRENT.BR_TAKEN) {
                unconditional_branching();
            } else {
                printf("BLE: Branch not taken, continuing sequentially.\n");
            }
            break;
        case BGT:
            if (MEM_to_WB_CURRENT.BR_TAKEN) {
                unconditional_branching();
            } else {
                printf("BGT: Branch not taken, continuing sequentially.\n");
            }
            break;
        case BGE:
            if (MEM_to_WB_CURRENT.BR_TAKEN) {
                unconditional_branching();
            } else {
                printf("BGE: Branch not taken, continuing sequentially.\n");    
    }
            break;
}
}

void pipe_stage_execute()
{
    memset(&EX_to_MEM_CURRENT, 0, sizeof(EX_to_MEM_CURRENT));
    Pipe_Op in = DE_to_EX_PREV;

    if (UPDATE_EX) {
        printf("=== EXECUTE STAGE - USING SAVED INSTRUCTION ===\n");
        in = SAVED_INSTRUCTION;
        UPDATE_EX = 0;
        printf("Executing saved instruction %d\n", in.INSTRUCTION);
    } else {
        printf("=== EXECUTE STAGE ===\n");
    }
    if (in.NOP) { EX_to_MEM_CURRENT.NOP = 1; printf("NOP in execute stage \n"); return; }
    EX_to_MEM_CURRENT = in;

    // Debug: Print what instruction we're executing
    printf("=== EXECUTE STAGE ===\n");
    printf("Instruction: %d, RD=%d, RN=%d(0x%lx), RM=%d(0x%lx), IMM=0x%lx\n", 
           in.INSTRUCTION, in.RD_REG, in.RN_REG, in.RN_VAL, in.RM_REG, in.RM_VAL, in.IMM);

    // Forward RN_VAL
    if (in.READS_RN) {
        printf("Instruction needs RN (reg %d = 0x%lx)\n", in.RN_REG, in.RN_VAL);
        
        if (EX_to_MEM_PREV.WRITES_REG && 
            EX_to_MEM_PREV.RD_REG == in.RN_REG && 
            EX_to_MEM_PREV.RD_REG != 31) {
            printf("EX->EX Forward: RN reg %d gets 0x%lx (was 0x%lx)\n", 
                   in.RN_REG, EX_to_MEM_PREV.result, in.RN_VAL);
            EX_to_MEM_CURRENT.RN_VAL = EX_to_MEM_PREV.result;
        }
        else if (MEM_to_WB_PREV.WRITES_REG && 
                 MEM_to_WB_PREV.RD_REG == in.RN_REG && 
                 MEM_to_WB_PREV.RD_REG != 31) {
            printf("MEM->EX Forward: RN reg %d gets 0x%lx (was 0x%lx)\n", 
                   in.RN_REG, MEM_to_WB_PREV.result, in.RN_VAL);
            EX_to_MEM_CURRENT.RN_VAL = MEM_to_WB_PREV.result;
        } 
        else if (MEM_to_WB_PREV.LOAD && 
                 MEM_to_WB_PREV.RT_REG == in.RN_REG && 
                 MEM_to_WB_PREV.RT_REG != 31) {
            printf("MEM->EX Forward (LOAD): RN reg %d gets 0x%lx (was 0x%lx)\n", 
                   in.RN_REG, MEM_to_WB_PREV.MEM_DATA, in.RN_VAL);
            EX_to_MEM_CURRENT.RN_VAL = MEM_to_WB_PREV.MEM_DATA;
        }
    }

    // Forward RM_VAL
    if (in.READS_RM) {
        printf("Instruction needs RM (reg %d = 0x%lx)\n", in.RM_REG, in.RM_VAL);
        
        // Check EX/MEM forwarding
        if (EX_to_MEM_PREV.WRITES_REG && 
            EX_to_MEM_PREV.RD_REG == in.RM_REG && 
            EX_to_MEM_PREV.RD_REG != 31) {
            printf("EX->EX Forward: RM reg %d gets 0x%lx (was 0x%lx)\n", 
                in.RM_REG, EX_to_MEM_PREV.result, in.RM_VAL);
            EX_to_MEM_CURRENT.RM_VAL = EX_to_MEM_PREV.result;
        }
        // Check MEM/WB forwarding
        else if (MEM_to_WB_PREV.WRITES_REG && 
                MEM_to_WB_PREV.RD_REG == in.RM_REG && 
                MEM_to_WB_PREV.RD_REG != 31) {
            printf("MEM->EX Forward: RM reg %d gets 0x%lx (was 0x%lx)\n", 
                in.RM_REG, MEM_to_WB_PREV.result, in.RM_VAL);
            EX_to_MEM_CURRENT.RM_VAL = MEM_to_WB_PREV.result;
        }
        else if (MEM_to_WB_PREV.LOAD && 
                 MEM_to_WB_PREV.RT_REG == in.RM_REG && 
                 MEM_to_WB_PREV.RT_REG != 31) {
            printf("MEM->EX Forward (LOAD): RM reg %d gets 0x%lx (was 0x%lx)\n", 
                   in.RM_REG, MEM_to_WB_PREV.MEM_DATA, in.RM_VAL);
            EX_to_MEM_CURRENT.RM_VAL = MEM_to_WB_PREV.MEM_DATA;
        }
    }
    // Forward RT_VAL for STORE
    if (in.STORE) {
    if (EX_to_MEM_PREV.WRITES_REG &&
        EX_to_MEM_PREV.RD_REG == in.RT_REG &&
        in.RT_REG != 31) {
        EX_to_MEM_CURRENT.RT_VAL = EX_to_MEM_PREV.result;
    }
    else if (MEM_to_WB_PREV.WRITES_REG &&
             MEM_to_WB_PREV.RD_REG == in.RT_REG &&
             in.RT_REG != 31) {
        EX_to_MEM_CURRENT.RT_VAL = MEM_to_WB_PREV.result;
    }
    else if (MEM_to_WB_PREV.LOAD &&
             MEM_to_WB_PREV.RT_REG == in.RT_REG &&
             in.RT_REG != 31) {
        EX_to_MEM_CURRENT.RT_VAL = MEM_to_WB_PREV.MEM_DATA;
    }
}

    // Debug: Print values before calculation
    printf("Before calc: RN_VAL=0x%lx, RM_VAL=0x%lx, IMM=0x%lx\n", 
           EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.RM_VAL, EX_to_MEM_CURRENT.IMM);

    switch (in.INSTRUCTION) {
        case ADD_EXT:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.RM_VAL;
            printf("ADD_EXT: 0x%lx + 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.RM_VAL, EX_to_MEM_CURRENT.result);
            break;
        case ADD_IMM:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            printf("ADD_IMM: 0x%lx + 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.IMM, EX_to_MEM_CURRENT.result);
            break;
        case ADDS_IMM:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            printf("ADDS_IMM: 0x%lx + 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.IMM, EX_to_MEM_CURRENT.result);
            break;
        case ADDS_EXT:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.RM_VAL;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            printf("ADDS_EXT: 0x%lx + 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.RM_VAL, EX_to_MEM_CURRENT.result);
            break;
        
        case AND_SHIFTR:
            EX_to_MEM_CURRENT.result = (EX_to_MEM_CURRENT.RN_VAL & EX_to_MEM_CURRENT.RM_VAL);
            printf("AND_SHIFTR: 0x%lx & 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.RM_VAL, EX_to_MEM_CURRENT.result);
            break;
        case ANDS_SHIFTR:
            EX_to_MEM_CURRENT.result = (EX_to_MEM_CURRENT.RN_VAL & EX_to_MEM_CURRENT.RM_VAL);
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            printf("ANDS_SHIFTR: 0x%lx & 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.RM_VAL, EX_to_MEM_CURRENT.result);
            break;
        
        case EOR_SHIFTR:
            EX_to_MEM_CURRENT.result = (EX_to_MEM_CURRENT.RN_VAL ^ EX_to_MEM_CURRENT.RM_VAL);
            printf("EOR_SHIFTR: 0x%lx ^ 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.RM_VAL, EX_to_MEM_CURRENT.result);
            break;
        case LSL_IMM:
        {
            uint32_t actual_shift = (64 - EX_to_MEM_CURRENT.SHAM) % 64;
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL << actual_shift;
            printf("LSL_IMM: 0x%lx << %d = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, actual_shift, EX_to_MEM_CURRENT.result);
            break;
        }
        case LSR_IMM:
        {
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL >> EX_to_MEM_CURRENT.SHAM;
            printf("LSR_IMM: 0x%lx >> %d = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.SHAM, EX_to_MEM_CURRENT.result);
            break;
        }
        case LDURB:
        {
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            printf("LDURB: mem_addr = 0x%lx + 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.IMM, EX_to_MEM_CURRENT.MEM_ADDRESS);
            break;
        }
        case MOVZ:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.IMM;
            printf("MOVZ: result = 0x%lx\n", EX_to_MEM_CURRENT.result);
            break;
        case MUL:
        {
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL * EX_to_MEM_CURRENT.RM_VAL;
            printf("MUL: 0x%lx * 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.RM_VAL, EX_to_MEM_CURRENT.result);
            break; 
        }
        case SUB_IMM:
        {
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.IMM;
            printf("SUB_IMM: 0x%lx - 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.IMM, EX_to_MEM_CURRENT.result);
            break;
        }
        case SUB_EXT:
        {
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.RM_VAL;
            printf("SUB_EXT: 0x%lx - 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.RM_VAL, EX_to_MEM_CURRENT.result);
            break; 
        }
        case SUBS_IMM:
        {
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.IMM;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            printf("SUBS_IMM: 0x%lx - 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.IMM, EX_to_MEM_CURRENT.result);
            break;
        }
        case SUBS_EXT:
        {
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.RM_VAL;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            printf("SUBS_EXT: 0x%lx - 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.RM_VAL, EX_to_MEM_CURRENT.result);
            break; 
        }
        case HLT:
            printf("HLT instruction\n");
            break;
        case STUR_32:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            printf("STUR_32: mem_addr = 0x%lx + 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.IMM, EX_to_MEM_CURRENT.MEM_ADDRESS);
            break;
        case STUR_64:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            printf("STUR_64: mem_addr = 0x%lx + 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.IMM, EX_to_MEM_CURRENT.MEM_ADDRESS);
            break;
        case STURB:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            printf("STURB: mem_addr = 0x%lx + 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.IMM, EX_to_MEM_CURRENT.MEM_ADDRESS);
            break;
        case STURH:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            printf("STURH: mem_addr = 0x%lx + 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.IMM, EX_to_MEM_CURRENT.MEM_ADDRESS);
            break;
        case LDUR_32:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            printf("LDUR_32: mem_addr = 0x%lx + 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.IMM, EX_to_MEM_CURRENT.MEM_ADDRESS);
            break;
        case LDUR_64:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            printf("LDUR_64: mem_addr = 0x%lx + 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.IMM, EX_to_MEM_CURRENT.MEM_ADDRESS);
            break;
        case LDURH:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            printf("LDURH: mem_addr = 0x%lx + 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.IMM, EX_to_MEM_CURRENT.MEM_ADDRESS);
            break;
        case CMP_EXT:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.RM_VAL;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = ((EX_to_MEM_CURRENT.result) >> 63) & 1;
            printf("CMP_EXT: 0x%lx - 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.RM_VAL, EX_to_MEM_CURRENT.result);
            break;
        case CMP_IMM:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.IMM;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = ((EX_to_MEM_CURRENT.result) >> 63) & 1;
            printf("CMP_IMM: 0x%lx - 0x%lx = 0x%lx\n", 
                   EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.IMM, EX_to_MEM_CURRENT.result);
            break;
        case B:
            EX_to_MEM_CURRENT.BR_TARGET = in.PC + (in.IMM << 2);
            printf("B: Branch target = 0x%lx + (0x%lx << 2) = 0x%lx\n", 
                in.PC, in.IMM, EX_to_MEM_CURRENT.BR_TARGET);
            break;
        case BR:
            EX_to_MEM_CURRENT.BR_TARGET = EX_to_MEM_CURRENT.RN_VAL;
            break;
        case BEQ:
            EX_to_MEM_CURRENT.BR_TARGET = EX_to_MEM_CURRENT.PC + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN = EX_to_MEM_PREV.FLAG_Z;
            printf("B: Branch target = 0x%lx + (0x%lx << 2) = 0x%lx\n", 
                in.PC, in.IMM, EX_to_MEM_CURRENT.BR_TARGET);
            break;
        case BNE:
            EX_to_MEM_CURRENT.BR_TARGET = EX_to_MEM_CURRENT.PC + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN = !EX_to_MEM_PREV.FLAG_Z;
            printf("B: Branch target = 0x%lx + (0x%lx << 2) = 0x%lx\n", 
                in.PC, in.IMM, EX_to_MEM_CURRENT.BR_TARGET);
            break;
        case BLT:
            EX_to_MEM_CURRENT.BR_TARGET = EX_to_MEM_CURRENT.PC + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN = EX_to_MEM_PREV.FLAG_N;
            printf("B: Branch target = 0x%lx + (0x%lx << 2) = 0x%lx\n", 
                in.PC, in.IMM, EX_to_MEM_CURRENT.BR_TARGET);
            break;
        case BLE:
            EX_to_MEM_CURRENT.BR_TARGET = EX_to_MEM_CURRENT.PC + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN = EX_to_MEM_PREV.FLAG_N || EX_to_MEM_PREV.FLAG_Z;
            printf("B: Branch target = 0x%lx + (0x%lx << 2) = 0x%lx\n", 
                in.PC, in.IMM, EX_to_MEM_CURRENT.BR_TARGET);
            break;
        case BGT:
            EX_to_MEM_CURRENT.BR_TARGET = EX_to_MEM_CURRENT.PC + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN = !EX_to_MEM_PREV.FLAG_N && !EX_to_MEM_PREV.FLAG_Z;
            printf("B: Branch target = 0x%lx + (0x%lx << 2) = 0x%lx\n", 
                in.PC, in.IMM, EX_to_MEM_CURRENT.BR_TARGET);
            break;
        case BGE:
            EX_to_MEM_CURRENT.BR_TARGET = EX_to_MEM_CURRENT.PC + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN = !EX_to_MEM_PREV.FLAG_N;
            printf("B: Branch target = 0x%lx + (0x%lx << 2) = 0x%lx\n", 
                in.PC, in.IMM, EX_to_MEM_CURRENT.BR_TARGET);
            break;
            
        default:
            printf("UNKNOWN INSTRUCTION: %d\n", EX_to_MEM_CURRENT.INSTRUCTION);
            break;
    }
    
    printf("Final result: 0x%lx\n", EX_to_MEM_CURRENT.result);
    printf("========================\n");
}

void pipe_stage_decode()
{
    memset(&DE_to_EX_CURRENT, 0, sizeof(DE_to_EX_CURRENT));

    Pipe_Op in = IF_to_DE_PREV;
    if (in.NOP) { DE_to_EX_CURRENT.NOP = 1; return; }

    if (CLEAR_DE) {
        DE_to_EX_CURRENT.NOP = 1;
        CLEAR_DE = 0; 
        printf("DE stage cleared due to branch\n");
        return;
    }
    uint64_t current_instruction = in.raw_instruction;

    DE_to_EX_CURRENT.PC = in.PC; 

    //ADD(EXTENDED) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x458)) {
        DE_to_EX_CURRENT.INSTRUCTION = ADD_EXT;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }

    // ADD_IMM
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x91)) {
        DE_to_EX_CURRENT.INSTRUCTION = ADD_IMM;
        DE_to_EX_CURRENT.RD_REG  = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG  = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL  = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.IMM  = extract_bits(current_instruction, 10, 21);
    }

    //ADDS(EXTENDED) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x558)) {
        DE_to_EX_CURRENT.INSTRUCTION = ADDS_EXT;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }

    //ADDS(IMM) - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xB1)){
        DE_to_EX_CURRENT.INSTRUCTION = ADDS_IMM; 
        DE_to_EX_CURRENT.RD_REG= extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 10, 21);
        DE_to_EX_CURRENT.IMM = immediate;
    }

    //CBNZ - K

    //CBZ - F

    //AND(SHIFTED REG) - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x8A)){
        DE_to_EX_CURRENT.INSTRUCTION = AND_SHIFTR; 
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG); 
        DE_to_EX_CURRENT.READS_RM = 1;
    }

    //ANDS(SHIFTED REGISTER) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x750)) {
        DE_to_EX_CURRENT.INSTRUCTION = ANDS_SHIFTR;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }


    //EOR(SHIFTED REG) - K
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xCA)) {
        DE_to_EX_CURRENT.INSTRUCTION = EOR_SHIFTR;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }

    //ORR (SHIFTED REG) - F - orr.s

    //LDUR (32-AND 64-BIT) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x5C2)) {
        DE_to_EX_CURRENT.INSTRUCTION = LDUR_32;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        DE_to_EX_CURRENT.READ_MEM = 1;
        DE_to_EX_CURRENT.LOAD = 1;
    }

    if (!(extract_bits(current_instruction, 21, 31) ^ 0x7C2)) {
        DE_to_EX_CURRENT.INSTRUCTION = LDUR_64;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        DE_to_EX_CURRENT.READ_MEM = 1;
        DE_to_EX_CURRENT.LOAD = 1;
    }

    //LDURB - F
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x1C2)){
        DE_to_EX_CURRENT.INSTRUCTION = LDURB;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20); 
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 12, 20); 
        DE_to_EX_CURRENT.READ_MEM = 1;
        DE_to_EX_CURRENT.LOAD = 1;
    }

    //LDURH - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x3C2)) {
        DE_to_EX_CURRENT.INSTRUCTION = LDURH;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        DE_to_EX_CURRENT.READ_MEM = 1;
        DE_to_EX_CURRENT.LOAD = 1;
    }

    //LSL(IMM) - F
    if ((!(extract_bits(current_instruction, 22, 31) ^ 0x34D)) && ((extract_bits(current_instruction, 10, 15) != 0x3F))){
 
        DE_to_EX_CURRENT.INSTRUCTION = LSL_IMM;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9); 
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.SHAM = extract_bits(current_instruction, 16, 21);
    }

    //LSR (IMM) - K
    if (!(extract_bits(current_instruction, 22, 31) ^ 0x34D) && !(extract_bits(current_instruction, 10, 15) ^ 0x3F)){
        DE_to_EX_CURRENT.INSTRUCTION = LSR_IMM;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.SHAM = extract_bits(current_instruction, 16, 21);
    }

    // MOVZ
    if (!(extract_bits(current_instruction, 23, 30) ^ 0xA5)) {
        DE_to_EX_CURRENT.INSTRUCTION = MOVZ;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.IMM    = (uint32_t)extract_bits(current_instruction, 5, 20);
    }

    //STUR (32-AND 64-BIT VAR) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x5C0)) {
        DE_to_EX_CURRENT.INSTRUCTION = STUR_32;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        DE_to_EX_CURRENT.STORE = 1;
    }
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x7C0)) {
        DE_to_EX_CURRENT.INSTRUCTION = STUR_64;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        DE_to_EX_CURRENT.STORE = 1;
    }

    //STURB - F
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x1C0)){
        DE_to_EX_CURRENT.INSTRUCTION = STURB;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9); 
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 12, 20);
        DE_to_EX_CURRENT.STORE = 1;
    }

    //STURH - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x3C0)) {
        DE_to_EX_CURRENT.INSTRUCTION = STURH;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        DE_to_EX_CURRENT.STORE = 1;
    }

    //SUB(IMM) - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xD1)){
        DE_to_EX_CURRENT.INSTRUCTION = SUB_IMM;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 10, 21);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 10, 21); 
    }

    //SUB (EXT) - F
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x658)){
        DE_to_EX_CURRENT.INSTRUCTION = SUB_EXT;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9); 
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG  = extract_bits(current_instruction, 16, 20); 
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }

    //SUBS - K
    if (!(extract_bits(current_instruction, 22, 31) ^ 0x3C4) && (extract_bits(current_instruction, 0, 4) != 31)){
        DE_to_EX_CURRENT.INSTRUCTION = SUBS_IMM;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 10, 21);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 11);
    }

    //SUBS (EXT) K :)
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x758) && (extract_bits(current_instruction, 0, 4) != 31)) {
        DE_to_EX_CURRENT.INSTRUCTION = SUBS_EXT;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }


    //MUL - F - mul.s
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x4D8)){
        DE_to_EX_CURRENT.INSTRUCTION = MUL;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9); 
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG  = extract_bits(current_instruction, 16, 20); 
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }


    // HLT
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x6a2)) {
        DE_to_EX_CURRENT.INSTRUCTION = HLT;
        HLT_NEXT = 1;
    }
    //CMP (EXT) K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x758)  && (extract_bits(current_instruction, 0, 4) == 31)) {
        DE_to_EX_CURRENT.INSTRUCTION = CMP_EXT;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }

    // CMP (IMM) K
    if (!(extract_bits(current_instruction, 22, 31) ^ 0x3c4) && (extract_bits(current_instruction, 0, 4) == 31)){
        DE_to_EX_CURRENT.INSTRUCTION = CMP_IMM;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 10, 21);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 11);
    }

    //BR - K -
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x6B0)){
        DE_to_EX_CURRENT.INSTRUCTION = BR;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1; 
        DE_to_EX_CURRENT.UBRANCH = 1; 
    }


    //B - F
    if (!(extract_bits(current_instruction, 26, 31) ^ 0x5)){
        DE_to_EX_CURRENT.INSTRUCTION = B;
        uint32_t immediate = extract_bits(current_instruction, 0, 25);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 25);
        DE_to_EX_CURRENT.UBRANCH = 1;
    }

    //BEQ - K
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x54) && (extract_bits(current_instruction, 0, 3) == 0x0)){
        DE_to_EX_CURRENT.INSTRUCTION = BEQ;
        uint32_t immediate = extract_bits(current_instruction, 5, 23);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 18);
        DE_to_EX_CURRENT.CBRANCH = 1;
    }


    //BNE, BLT, BLE - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x54)){
        unsigned int cond_code = extract_bits(current_instruction, 0, 3);
        uint32_t immediate = extract_bits(current_instruction, 5, 23);
        DE_to_EX_CURRENT.IMM  = bit_extension(immediate, 5, 23);
        DE_to_EX_CURRENT.CBRANCH = 1;

        switch (cond_code){
            case 0x1:
                DE_to_EX_CURRENT.INSTRUCTION = BNE;
                break;
            case 0xB:
                DE_to_EX_CURRENT.INSTRUCTION = BLT;
                break;
            case 0xD:
                DE_to_EX_CURRENT.INSTRUCTION = BLE;
                break;
        }
    }

    //BGT - K
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x54) && !(extract_bits(current_instruction, 0, 3) == 0xC)){
        DE_to_EX_CURRENT.INSTRUCTION = BGT;
        uint32_t immediate = extract_bits(current_instruction, 5, 23);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 18);
        DE_to_EX_CURRENT.CBRANCH = 1;
    }

    //BGE K
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x54) && !(extract_bits(current_instruction, 0, 3) == 0xA)){
        DE_to_EX_CURRENT.INSTRUCTION = BGE;
        uint32_t immediate = extract_bits(current_instruction, 5, 23);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 18);
        DE_to_EX_CURRENT.CBRANCH = 1;
    }

    if (DE_to_EX_CURRENT.UBRANCH) {
        printf("Unconditional branch detected in DE stage - initiating branch sequence\n");
        printf("Branch instruction: 0x%08x at PC=0x%lx\n", current_instruction, DE_to_EX_CURRENT.PC);
        REFETCH_ADDR     = IF_to_DE_PREV.PC + 4;    // re-fetch SAME instruction as in N
        REFETCH_PC_NEXT  = 1;                   // triggers in fetch at N+1
        CLEAR_DE         = 1;
        BRANCH_NEXT = 1;                   // bubble DE at N+1
        return;
    }

    if (DE_to_EX_CURRENT.CBRANCH) {
        printf("Conditional branch detected in DE stage - initiating branch sequence\n");
        printf("Branch instruction: 0x%08x at PC=0x%lx\n", current_instruction, DE_to_EX_CURRENT.PC);
        REFETCH_ADDR     = IF_to_DE_PREV.PC + 4;    // re-fetch SAME instruction as in N
        REFETCH_PC_NEXT  = 1;                   // triggers in fetch at N+1
        CLEAR_DE         = 1;
        BRANCH_NEXT = 1;                   // bubble DE at N+1
        return;
    }
    printf("DE stage decoded instruction: %d\n", DE_to_EX_CURRENT.INSTRUCTION);
    printf("=== HAZARD CHECK DEBUG ===\n");
    printf("DE_to_EX_PREV (currently in EX): inst=%d, LOAD=%d, RT_REG=%d, NOP=%d\n", 
           DE_to_EX_PREV.INSTRUCTION, DE_to_EX_PREV.LOAD, DE_to_EX_PREV.RT_REG, DE_to_EX_PREV.NOP);
    printf("Current decode: READS_RN=%d (X%d), READS_RM=%d (X%d), STORE=%d (X%d)\n",
           DE_to_EX_CURRENT.READS_RN, DE_to_EX_CURRENT.RN_REG,
           DE_to_EX_CURRENT.READS_RM, DE_to_EX_CURRENT.RM_REG, 
           DE_to_EX_CURRENT.STORE, DE_to_EX_CURRENT.RT_REG);

    if (DE_to_EX_PREV.LOAD && !DE_to_EX_PREV.NOP) {
        printf("LOAD in EX stage detected, checking for hazards...\n");
        
        int load_target = DE_to_EX_PREV.RT_REG;
        int need_stall = 0;
        
        if (DE_to_EX_CURRENT.READS_RN && DE_to_EX_CURRENT.RN_REG == load_target && load_target != 31) {
            printf("*** HAZARD: Current instruction needs X%d (RN) but load writing to X%d ***\n", 
                   DE_to_EX_CURRENT.RN_REG, load_target);
            need_stall = 1;
        }
        if (DE_to_EX_CURRENT.READS_RM && DE_to_EX_CURRENT.RM_REG == load_target && load_target != 31) {
            printf("*** HAZARD: Current instruction needs X%d (RM) but load writing to X%d ***\n", 
                   DE_to_EX_CURRENT.RM_REG, load_target);
            need_stall = 1;
        }
        if (DE_to_EX_CURRENT.STORE && DE_to_EX_CURRENT.RT_REG == load_target && load_target != 31) {
            printf("*** HAZARD: Store instruction needs X%d (RT) but load writing to X%d ***\n", 
                   DE_to_EX_CURRENT.RT_REG, load_target);
            need_stall = 1;
        }
        
        if (need_stall) {
            printf("*** LOAD-USE HAZARD DETECTED - INITIATING STALL SEQUENCE ***\n");

            // Save the consumer exactly as decoded in cycle N
            SAVED_INSTRUCTION = DE_to_EX_CURRENT;
            memset(&DE_to_EX_CURRENT, 0, sizeof(DE_to_EX_CURRENT));
            DE_to_EX_CURRENT.NOP = 1;

            // Ask IF to re-fetch the SAME instruction next cycle (N+1)
            REFETCH_ADDR     = IF_to_DE_PREV.PC + 4;
            REFETCH_PC_NEXT  = 1;

            // Make DE a NOP in N+1
            CLEAR_DE = 1;
            return;
        }

    }
}



void pipe_stage_fetch()
{  
    static Pipe_Op fetched_instruction;
    static int cycle_count = 0;
    cycle_count++;
    
    printf("FETCH CYCLE %d: PC=0x%lx, REFETCH_PC=0x%lx\n", cycle_count, pipe.PC, REFETCH_PC);
    
    if (REFETCH_PC) {
        printf("REFETCH - Re-fetching instruction from PC=0x%lx due to load stall\n", REFETCH_ADDR);

        memset(&fetched_instruction, 0, sizeof(Pipe_Op));
        fetched_instruction.raw_instruction = mem_read_32(REFETCH_ADDR);
        fetched_instruction.PC = REFETCH_ADDR;
        fetched_instruction.NOP = 0;
        fetched_instruction.INSTRUCTION = UNKNOWN;
        printf("REFETCH: Read 0x%08x from PC=0x%lx\n",
            fetched_instruction.raw_instruction, REFETCH_ADDR);
        REFETCH_PC = 0;
        IF_to_DE_CURRENT = fetched_instruction;
        if (!BRANCH) {
            UPDATE_EX_NEXT = 1;
        }
        return;
    }
    if (SQUASH) {
        if (HLT_FLAG){
            printf("SQUASH active -  PC at 0x%lx\n", pipe.PC);
            memset(&fetched_instruction, 0, sizeof(Pipe_Op));
            fetched_instruction.NOP = 1;
            fetched_instruction.INSTRUCTION = UNKNOWN;
            printf("HLT_FLAG set - inserting NOP\n");
        } else {
            memset(&fetched_instruction, 0, sizeof(Pipe_Op));
            
            pipe.PC = MEM_to_WB_CURRENT.BR_TARGET;
            printf("SQUASH: Setting PC to branch target 0x%lx\n", pipe.PC);
            fetched_instruction.raw_instruction = mem_read_32(pipe.PC);
            fetched_instruction.PC = pipe.PC;
            fetched_instruction.NOP = 0;
            fetched_instruction.INSTRUCTION = UNKNOWN;
            printf("FETCH: Read 0x%08x from PC=0x%lx (after branch)\n", fetched_instruction.raw_instruction, pipe.PC);
            pipe.PC = pipe.PC + 4;
            printf("FETCH: Advanced PC to 0x%lx\n", pipe.PC);
        }
        SQUASH = 0;  // Clear squash flag
        IF_to_DE_CURRENT = fetched_instruction;
        return; 
    }

    if (!HLT_FLAG) {
        memset(&fetched_instruction, 0, sizeof(Pipe_Op));
        uint32_t raw_inst = mem_read_32(pipe.PC);
        printf("FETCH: Read 0x%08x from PC=0x%lx\n", raw_inst, pipe.PC);
        fetched_instruction.raw_instruction = raw_inst;
        fetched_instruction.PC = pipe.PC;
        fetched_instruction.NOP = 0;
        fetched_instruction.INSTRUCTION = UNKNOWN;
        pipe.PC = pipe.PC + 4; 
        printf("FETCH: Advanced PC to 0x%lx\n", pipe.PC);
    } else {
        memset(&fetched_instruction, 0, sizeof(Pipe_Op));
        fetched_instruction.NOP = 1;
        fetched_instruction.INSTRUCTION = UNKNOWN;
        printf("HLT_FLAG set - inserting NOP\n");
    }

    IF_to_DE_CURRENT = fetched_instruction;
}