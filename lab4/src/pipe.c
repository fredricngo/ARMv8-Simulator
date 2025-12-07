
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
# include "cache.h"

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
int UPDATE_EX = 0;
int UPDATE_EX_NEXT = 0;

int HLT_FLAG = 0;
int HLT_NEXT = 0;
int RUN_BIT;
uint64_t NEXT_PC;

int CLEAR_DE = 0;
int BRANCH = 0;
int BRANCH_NEXT = 0;

int DCACHE_MISS = 0; 
int DCACHE_MISS_CYCLES_REMAINING = 0;
uint64_t DCACHE_MISS_ADDR = 0;
Pipe_Op DCACHE_STALLED_OP;
cache_t *instruction_cache = NULL;
cache_t *data_cache = NULL;

int ICACHE_MISS = 0;
int ICACHE_MISS_CYCLES_REMAINING = 0;
uint64_t ICACHE_MISS_PC = 0;

int ICACHE_MISS_CANCELLED = 0;
int ICACHE_MISS_CANCEL_DELAY = 0;
int DCACHE_STALLED_THIS_CYCLE = 0;

int LOAD_STALL = 0;

static void set_nop(Pipe_Op *op)
{
    memset(op, 0, sizeof(Pipe_Op));
    op->NOP = 1;
    op->INSTRUCTION = UNKNOWN;
}

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
void pipe_init()
{
    memset(&pipe, 0, sizeof(Pipe_State));
    pipe.PC = 0x00400000;
    RUN_BIT = TRUE;
    NEXT_PC = pipe.PC;

    bp_t_init();
    pipe.bp = &bp;

    instruction_cache = cache_new(64, 4, 32);
    data_cache        = cache_new(256, 8, 32);

    set_nop(&IF_to_DE_CURRENT);
    set_nop(&DE_to_EX_CURRENT);
    set_nop(&EX_to_MEM_CURRENT);
    set_nop(&MEM_to_WB_CURRENT);

    set_nop(&IF_to_DE_PREV);
    set_nop(&DE_to_EX_PREV);
    set_nop(&EX_to_MEM_PREV);
    set_nop(&MEM_to_WB_PREV);

    HLT_FLAG = 0;
    HLT_NEXT = 0;
    CLEAR_DE = 0;
}

void pipe_cycle()
{
    printf("\n[CYCLE START] ============================================\n");
    printf("[CYCLE START] IF_to_DE_PREV: PC=0x%lx, NOP=%d, INST=%d\n", 
           IF_to_DE_PREV.PC, IF_to_DE_PREV.NOP, IF_to_DE_PREV.INSTRUCTION);
    printf("[CYCLE START] DE_to_EX_PREV: PC=0x%lx, NOP=%d, INST=%d, LOAD=%d, RT_REG=%d\n", 
           DE_to_EX_PREV.PC, DE_to_EX_PREV.NOP, DE_to_EX_PREV.INSTRUCTION, 
           DE_to_EX_PREV.LOAD, DE_to_EX_PREV.RT_REG);
    printf("[CYCLE START] EX_to_MEM_PREV: PC=0x%lx, NOP=%d, INST=%d, LOAD=%d, RT_REG=%d\n", 
           EX_to_MEM_PREV.PC, EX_to_MEM_PREV.NOP, EX_to_MEM_PREV.INSTRUCTION,
           EX_to_MEM_PREV.LOAD, EX_to_MEM_PREV.RT_REG);
    printf("[CYCLE START] MEM_to_WB_PREV: PC=0x%lx, NOP=%d, INST=%d, LOAD=%d, RT_REG=%d, MEM_DATA=0x%lx\n", 
           MEM_to_WB_PREV.PC, MEM_to_WB_PREV.NOP, MEM_to_WB_PREV.INSTRUCTION,
           MEM_to_WB_PREV.LOAD, MEM_to_WB_PREV.RT_REG, MEM_to_WB_PREV.MEM_DATA);
    
    if (DCACHE_MISS_CYCLES_REMAINING > 0) {
        DCACHE_MISS_CYCLES_REMAINING--; 
        printf("[CYCLE] D-cache stall, cycles remaining: %d\n", DCACHE_MISS_CYCLES_REMAINING);
        
        // Tick I-cache counter during D-cache stall, but don't resolve here
        if (ICACHE_MISS && ICACHE_MISS_CYCLES_REMAINING > 0) {
            ICACHE_MISS_CYCLES_REMAINING--;
            printf("[CYCLE] I-cache miss ticking during D-cache stall, cycles remaining: %d\n", 
                   ICACHE_MISS_CYCLES_REMAINING);
            // Don't resolve here - let fetch handle resolution when it runs
        }
    } else {
        pipe_stage_wb();
        pipe_stage_mem();
        pipe_stage_execute();
        pipe_stage_decode();
        pipe_stage_fetch(); 
        
        if (DCACHE_MISS == 1) {
            DCACHE_MISS_CYCLES_REMAINING = 49;
            MEM_to_WB_PREV.NOP = 1;
        } else if (LOAD_STALL) {
            MEM_to_WB_PREV = MEM_to_WB_CURRENT;
            EX_to_MEM_PREV = EX_to_MEM_CURRENT;
        } else {
            MEM_to_WB_PREV = MEM_to_WB_CURRENT;
            EX_to_MEM_PREV = EX_to_MEM_CURRENT;
            DE_to_EX_PREV = DE_to_EX_CURRENT;
            IF_to_DE_PREV = IF_to_DE_CURRENT;
            pipe.PC = NEXT_PC;
        }
    }
    
    printf("[AFTER STAGES] LOAD_STALL=%d\n", LOAD_STALL);
    printf("[AFTER STAGES] DE_to_EX_CURRENT: PC=0x%lx, NOP=%d, INST=%d\n", 
           DE_to_EX_CURRENT.PC, DE_to_EX_CURRENT.NOP, DE_to_EX_CURRENT.INSTRUCTION);
    printf("[AFTER STAGES] EX_to_MEM_CURRENT: PC=0x%lx, NOP=%d, INST=%d\n", 
           EX_to_MEM_CURRENT.PC, EX_to_MEM_CURRENT.NOP, EX_to_MEM_CURRENT.INSTRUCTION);
    printf("[AFTER STAGES] MEM_to_WB_CURRENT: PC=0x%lx, NOP=%d, INST=%d, MEM_DATA=0x%lx\n", 
           MEM_to_WB_CURRENT.PC, MEM_to_WB_CURRENT.NOP, MEM_to_WB_CURRENT.INSTRUCTION,
           MEM_to_WB_CURRENT.MEM_DATA);

    UPDATE_EX = UPDATE_EX_NEXT;
    UPDATE_EX_NEXT = 0;
    BRANCH = BRANCH_NEXT;
    BRANCH_NEXT = 0;
    HLT_FLAG = HLT_NEXT;
    CLEAR_DE = 0;
    LOAD_STALL = 0;
    printf("[CYCLE END] ==============================================\n\n");
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

    if (in.NOP || in.INSTRUCTION == UNKNOWN) {
        return;
    }

    /* 1. Handle flags and HLT side effects */
    switch (in.INSTRUCTION) {
        case ADDS_IMM:
        case ADDS_EXT:
        case SUBS_IMM:
        case SUBS_EXT:
        case CMP_IMM:
        case CMP_EXT:
            // These instructions compute flags in EX; just commit them here.
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
            break;

        case HLT:
            // Let HLT retire like a normal instruction, then stop the run loop.
            RUN_BIT = 0;
            break;

        default:
            // No special side effects.
            break;
    }

    /* 2. Perform register writes */

    if (in.LOAD) {
        // All loads write RT from MEM_DATA
        if (in.RT_REG != 31) {
            if (in.INSTRUCTION == LDUR_64) {
                printf("[WB] LDUR_64: writing MEM_DATA=0x%lx to RT_REG=%d\n",
                       in.MEM_DATA, in.RT_REG);
            }
            write_register(in.RT_REG, in.MEM_DATA);
        }
    } else if (in.WRITES_REG) {
        // All ALU/MOV/etc instructions write RD from result
        if (in.RD_REG != 31) {
            write_register(in.RD_REG, in.result);
        }
    }
    // Stores / branches / pure flag-setters do not write registers here.

    /* 3. Count a retired instruction for every real op */
    stat_inst_retire++;
}


void pipe_stage_mem()
{
    printf("[MEM] DCACHE_MISS=%d, DCACHE_STALLED_THIS_CYCLE=%d, cycles_remaining=%d\n",
           DCACHE_MISS, DCACHE_STALLED_THIS_CYCLE, DCACHE_MISS_CYCLES_REMAINING);

    memset(&MEM_to_WB_CURRENT, 0, sizeof(MEM_to_WB_CURRENT));
    
    // If we're in a D-cache miss, use the stalled op; otherwise use normal input
    Pipe_Op in = EX_to_MEM_PREV;
    
    if (in.NOP) { 
        MEM_to_WB_CURRENT.NOP = 1; 
        return; 
    }

    
    if (DCACHE_MISS && DCACHE_MISS_CYCLES_REMAINING == 0) {
        printf("[MEM] D-cache MISS resolved at addr 0x%lx\n", DCACHE_MISS_ADDR);
        cache_insert(data_cache, DCACHE_MISS_ADDR);
        DCACHE_MISS = 0;
    } else if (!DCACHE_MISS) {
        int hit = cache_check(data_cache, in.MEM_ADDRESS);
        if (!hit && (in.LOAD || in.STORE)) {
            printf("[MEM] D-cache MISS at addr 0x%lx, starting 50 cycle stall\n", in.MEM_ADDRESS);
            DCACHE_MISS = 1;
            DCACHE_MISS_ADDR = in.MEM_ADDRESS;
            return;
        }
    }

    MEM_to_WB_CURRENT = in;

    // Perform the actual memory access
    switch (in.INSTRUCTION) {
        case STUR_32:
            printf("[MEM] STUR_32: writing 0x%x to address 0x%lx\n", 
               (uint32_t)(in.RT_VAL & 0xFFFFFFFF), in.MEM_ADDRESS);
            mem_write_32(in.MEM_ADDRESS, (uint32_t)(in.RT_VAL & 0xFFFFFFFF));
            break;
        case STUR_64:
            printf("[MEM] STUR_64: writing 0x%lx to address 0x%lx\n", 
               in.RT_VAL, in.MEM_ADDRESS);
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
        {
            printf("[MEM] LDUR_32: reading from address 0x%lx\n", in.MEM_ADDRESS);
            MEM_to_WB_CURRENT.MEM_DATA = (int32_t) mem_read_32(in.MEM_ADDRESS);
            printf("[MEM] LDUR_32: got value 0x%lx\n", MEM_to_WB_CURRENT.MEM_DATA);
            break;
        }
        case LDUR_64:
        {
            printf("[MEM] LDUR_64: reading from address 0x%lx\n", in.MEM_ADDRESS);
            uint32_t low_word = mem_read_32(in.MEM_ADDRESS);
            uint32_t high_word = mem_read_32(in.MEM_ADDRESS + 4);
            MEM_to_WB_CURRENT.MEM_DATA = ((uint64_t)high_word << 32) | low_word;
            printf("[MEM] LDUR_64: got value 0x%lx\n", MEM_to_WB_CURRENT.MEM_DATA);
            break;
        }
        case LDURH:
            MEM_to_WB_CURRENT.MEM_DATA = (int16_t) mem_read_32(in.MEM_ADDRESS);
            break;
        case LDURB:
        {
            uint32_t word = mem_read_32(in.MEM_ADDRESS & ~3);
            int byte_offset = in.MEM_ADDRESS & 3;
            uint8_t byte_data = (word >> (byte_offset * 8)) & 0xFF;
            MEM_to_WB_CURRENT.MEM_DATA = (uint64_t) byte_data;
            break;
        }
        default:
            break;
    }
}


void pipe_stage_execute()
{
    memset(&EX_to_MEM_CURRENT, 0, sizeof(EX_to_MEM_CURRENT));

    Pipe_Op in = DE_to_EX_PREV;

    if (UPDATE_EX) {
        in = SAVED_INSTRUCTION;
        UPDATE_EX = 0;
    }

    if (in.NOP) {
        EX_to_MEM_CURRENT.NOP = 1;
        return;
    }

    EX_to_MEM_CURRENT = in;

    uint64_t branch_pc = in.PC;


    // Forward RN_VAL
    if (in.READS_RN) {
        if (EX_to_MEM_PREV.WRITES_REG &&
            EX_to_MEM_PREV.RD_REG == in.RN_REG &&
            EX_to_MEM_PREV.RD_REG != 31) {

            EX_to_MEM_CURRENT.RN_VAL = EX_to_MEM_PREV.result;
        }
        else if (MEM_to_WB_PREV.WRITES_REG &&
                 MEM_to_WB_PREV.RD_REG == in.RN_REG &&
                 MEM_to_WB_PREV.RD_REG != 31) {

            EX_to_MEM_CURRENT.RN_VAL = MEM_to_WB_PREV.result;
        }
        else if (MEM_to_WB_PREV.LOAD &&
                 MEM_to_WB_PREV.RT_REG == in.RN_REG &&
                 MEM_to_WB_PREV.RT_REG != 31) {

            EX_to_MEM_CURRENT.RN_VAL = MEM_to_WB_PREV.MEM_DATA;
        }
    }

    // Forward RM_VAL
    if (in.READS_RM) {
        if (EX_to_MEM_PREV.WRITES_REG &&
            EX_to_MEM_PREV.RD_REG == in.RM_REG &&
            EX_to_MEM_PREV.RD_REG != 31) {

            EX_to_MEM_CURRENT.RM_VAL = EX_to_MEM_PREV.result;
        }
        else if (MEM_to_WB_PREV.WRITES_REG &&
                 MEM_to_WB_PREV.RD_REG == in.RM_REG &&
                 MEM_to_WB_PREV.RD_REG != 31) {

            EX_to_MEM_CURRENT.RM_VAL = MEM_to_WB_PREV.result;
        }
        else if (MEM_to_WB_PREV.LOAD &&
                 MEM_to_WB_PREV.RT_REG == in.RM_REG &&
                 MEM_to_WB_PREV.RT_REG != 31) {

            EX_to_MEM_CURRENT.RM_VAL = MEM_to_WB_PREV.MEM_DATA;
        }
    }

    // Forward for STORE
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

    // Forward for CBNZ / CBZ
    if (in.INSTRUCTION == CBNZ || in.INSTRUCTION == CBZ) {
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

    /************** ALU / FLAGS / ADDRESS CALC **************/

    switch (in.INSTRUCTION) {
        case ADD_EXT:
            printf("[EX] ADD: RN_REG=%d (val=0x%lx), RM_REG=%d (val=0x%lx)\n", 
            EX_to_MEM_CURRENT.RN_REG, EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.RM_REG, EX_to_MEM_CURRENT.RM_VAL);
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.RM_VAL;
            break;
        case ADD_IMM:
            printf("[EX] ADD: RN_REG=%d (val=0x%lx), RM_REG=%d (val=0x%lx)\n", 
                EX_to_MEM_CURRENT.RN_REG, EX_to_MEM_CURRENT.RN_VAL, EX_to_MEM_CURRENT.RM_REG, EX_to_MEM_CURRENT.RM_VAL);
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            break;
        case ADDS_IMM:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            break;
        case ADDS_EXT:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.RM_VAL;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            break;

        case AND_SHIFTR:
            EX_to_MEM_CURRENT.result = (EX_to_MEM_CURRENT.RN_VAL & EX_to_MEM_CURRENT.RM_VAL);
            break;
        case ANDS_SHIFTR:
            EX_to_MEM_CURRENT.result = (EX_to_MEM_CURRENT.RN_VAL & EX_to_MEM_CURRENT.RM_VAL);
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            break;

        case EOR_SHIFTR:
            EX_to_MEM_CURRENT.result = (EX_to_MEM_CURRENT.RN_VAL ^ EX_to_MEM_CURRENT.RM_VAL);
            break;

        case LSL_IMM: {
            uint32_t actual_shift = (64 - EX_to_MEM_CURRENT.SHAM) % 64;
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL << actual_shift;
            break;
        }
        case LSR_IMM:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL >> EX_to_MEM_CURRENT.SHAM;
            break;

        case LDURB:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            break;

        case MOVZ:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.IMM;
            break;

        case MUL:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL * EX_to_MEM_CURRENT.RM_VAL;
            break;

        case SUB_IMM:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.IMM;
            break;

        case SUB_EXT:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.RM_VAL;
            break;

        case SUBS_IMM:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.IMM;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            break;

        case SUBS_EXT:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.RM_VAL;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            break;

        case HLT:
            // nothing to do in EX
            break;

        case STUR_32:
        case STUR_64:
        case STURB:
        case STURH:
        case LDUR_32:
        case LDUR_64:
        case LDURH:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            break;

        case CMP_EXT:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.RM_VAL;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            break;

        case CMP_IMM:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.IMM;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            break;

        /************** BRANCH ADDRESS / TAKEN **************/

        case B:
            EX_to_MEM_CURRENT.BR_TARGET = branch_pc + (in.IMM << 2);
            break;

        case BR:
            EX_to_MEM_CURRENT.BR_TARGET = EX_to_MEM_CURRENT.RN_VAL;
            break;

        case BEQ:
            EX_to_MEM_CURRENT.BR_TARGET = branch_pc + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN  = EX_to_MEM_PREV.FLAG_Z;
            break;

        case BNE:
            EX_to_MEM_CURRENT.BR_TARGET = branch_pc + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN  = !EX_to_MEM_PREV.FLAG_Z;
            break;

        case BLT:
            EX_to_MEM_CURRENT.BR_TARGET = branch_pc + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN  = EX_to_MEM_PREV.FLAG_N;
            break;

        case BLE:
            EX_to_MEM_CURRENT.BR_TARGET = branch_pc + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN  = EX_to_MEM_PREV.FLAG_N || EX_to_MEM_PREV.FLAG_Z;
            break;

        case BGT:
            EX_to_MEM_CURRENT.BR_TARGET = branch_pc + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN  = !EX_to_MEM_PREV.FLAG_N && !EX_to_MEM_PREV.FLAG_Z;
            break;

        case BGE:
            EX_to_MEM_CURRENT.BR_TARGET = branch_pc + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN  = !EX_to_MEM_PREV.FLAG_N;
            break;

        case CBNZ:
            EX_to_MEM_CURRENT.BR_TARGET = branch_pc + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN  = (EX_to_MEM_CURRENT.RT_VAL != 0);
            break;

        case CBZ:
            EX_to_MEM_CURRENT.BR_TARGET = branch_pc + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN  = (EX_to_MEM_CURRENT.RT_VAL == 0);
            break;

        case ORR_SHIFTR:
            EX_to_MEM_CURRENT.result = (EX_to_MEM_CURRENT.RN_VAL | EX_to_MEM_CURRENT.RM_VAL);
            break;

        default:
            break;
    }

    /************** BRANCH RESOLUTION / SQUASH **************/

    if (EX_to_MEM_CURRENT.UBRANCH || EX_to_MEM_CURRENT.CBRANCH) {
        bp_update(&EX_to_MEM_CURRENT);

        uint64_t correct_pc;

        if (EX_to_MEM_CURRENT.UBRANCH) {
            correct_pc = EX_to_MEM_CURRENT.BR_TARGET;
        } else {
            // Conditional branch: either target or fall-through = branch_pc + 4
            correct_pc = EX_to_MEM_CURRENT.BR_TAKEN
                         ? EX_to_MEM_CURRENT.BR_TARGET
                         : (branch_pc + 4);
        }

        if (EX_to_MEM_CURRENT.PREDICTED_PC != correct_pc ||
            EX_to_MEM_CURRENT.BTB_MISS) {

            NEXT_PC = correct_pc;
            stat_squash++;
            CLEAR_DE = 1;

            memset(&IF_to_DE_CURRENT, 0, sizeof(IF_to_DE_CURRENT));
            IF_to_DE_CURRENT.NOP = 1;
        }
    }
}


void pipe_stage_decode()
{
    memset(&DE_to_EX_CURRENT, 0, sizeof(DE_to_EX_CURRENT));

    Pipe_Op in = IF_to_DE_PREV;
    if (in.NOP) { DE_to_EX_CURRENT.NOP = 1; return; }

    if (CLEAR_DE || HLT_FLAG) {
        DE_to_EX_CURRENT.NOP = 1;
        return;
    }
    uint64_t current_instruction = in.raw_instruction;

    DE_to_EX_CURRENT.PC = in.PC; 
    DE_to_EX_CURRENT.PREDICTED_PC = in.PREDICTED_PC;
    DE_to_EX_CURRENT.BTB_MISS     = in.BTB_MISS;
    DE_to_EX_CURRENT.GHR_XOR_PC   = in.GHR_XOR_PC;

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
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xB5)) {
        DE_to_EX_CURRENT.INSTRUCTION = CBNZ;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 5, 23);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 18);
        DE_to_EX_CURRENT.CBRANCH = 1;
    }

    //CBZ - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xB4)){
    DE_to_EX_CURRENT.INSTRUCTION = CBZ;
    DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
    DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
    uint32_t immediate = extract_bits(current_instruction, 5, 23);
    DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 18);
    DE_to_EX_CURRENT.CBRANCH = 1;
}


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
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xAA)){
    DE_to_EX_CURRENT.INSTRUCTION = ORR_SHIFTR;
    DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
    DE_to_EX_CURRENT.WRITES_REG = 1;
    DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
    DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
    DE_to_EX_CURRENT.READS_RN = 1;
    DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
    DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
    DE_to_EX_CURRENT.READS_RM = 1;
}

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

    //SUBS (EXT) K
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


    //MUL - F
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


    if (!(extract_bits(current_instruction, 24, 31) ^ 0x54)) {
        unsigned int cond_code = extract_bits(current_instruction, 0, 3);
        uint32_t imm19         = extract_bits(current_instruction, 5, 23);

        // imm19 is 19 bits (bits [23:5]) → sign-extend from bit 18
        int64_t imm_signed = bit_extension(imm19, 0, 18);

        DE_to_EX_CURRENT.CBRANCH = 1;
        DE_to_EX_CURRENT.IMM     = imm_signed;

        switch (cond_code) {
            case 0x0: // EQ
                DE_to_EX_CURRENT.INSTRUCTION = BEQ;
                break;
            case 0x1: // NE
                DE_to_EX_CURRENT.INSTRUCTION = BNE;
                break;
            case 0xB: // LT
                DE_to_EX_CURRENT.INSTRUCTION = BLT;
                break;
            case 0xD: // LE
                DE_to_EX_CURRENT.INSTRUCTION = BLE;
                break;
            case 0xC: // GT
                DE_to_EX_CURRENT.INSTRUCTION = BGT;
                break;
            case 0xA: // GE
                DE_to_EX_CURRENT.INSTRUCTION = BGE;
                break;
            default:
                // you can leave UNKNOWN for other conds you don't care about
                break;
        }
    }

        if (DE_to_EX_PREV.LOAD && !DE_to_EX_PREV.NOP) {
        int load_target = DE_to_EX_PREV.RT_REG;
        int need_stall = 0;
        
        printf("[DECODE] Checking hazard: DE_to_EX_PREV.PC=0x%lx, LOAD=%d, RT_REG=%d\n",
               DE_to_EX_PREV.PC, DE_to_EX_PREV.LOAD, DE_to_EX_PREV.RT_REG);
        printf("[DECODE] Current instruction PC=0x%lx, RN_REG=%d, RM_REG=%d\n",
               in.PC, DE_to_EX_CURRENT.RN_REG, DE_to_EX_CURRENT.RM_REG);
        
        if (DE_to_EX_CURRENT.READS_RN &&
            DE_to_EX_CURRENT.RN_REG == load_target &&
            load_target != 31) {
            need_stall = 1;
        }
        if (DE_to_EX_CURRENT.READS_RM &&
            DE_to_EX_CURRENT.RM_REG == load_target &&
            load_target != 31) {
            need_stall = 1;
        }
        if (DE_to_EX_CURRENT.STORE &&
            DE_to_EX_CURRENT.RT_REG == load_target &&
            load_target != 31) {
            need_stall = 1;
        }
        
        if (need_stall) {
            printf("[DECODE] Load-use hazard detected on R%d, stalling pipeline\n", load_target);

            // 1. Insert a bubble into EX
            memset(&DE_to_EX_CURRENT, 0, sizeof(DE_to_EX_CURRENT));
            DE_to_EX_CURRENT.NOP = 1;

            // 2. Freeze PC and IF->DE this cycle (handled in pipe_cycle)
            LOAD_STALL = 1;

            // IMPORTANT: do NOT set CLEAR_DE, do NOT touch REFETCH/UPDATE_EX here.
            return;
        }
    }
}

void pipe_stage_fetch()
{
    Pipe_Op fetched_instruction;
    memset(&fetched_instruction, 0, sizeof(Pipe_Op));
    fetched_instruction.NOP = 1;
    fetched_instruction.INSTRUCTION = UNKNOWN;

    if (HLT_FLAG) {
        IF_to_DE_CURRENT = fetched_instruction;
        return;
    }

    uint64_t fetch_pc = pipe.PC;

    if (CLEAR_DE) {
        fetch_pc = NEXT_PC;
    }

    uint64_t block_mask = ~((uint64_t)instruction_cache->block_size - 1);
    uint64_t miss_block  = ICACHE_MISS_PC & block_mask;
    uint64_t redirect_block = fetch_pc & block_mask;  // Use fetch_pc, not NEXT_PC

    printf("[FETCH] PC=0x%lx, MISS=%d, MISS_PC=0x%lx, CYCLES=%d, CLEAR_DE=%d\n",
           pipe.PC, ICACHE_MISS, ICACHE_MISS_PC, ICACHE_MISS_CYCLES_REMAINING, CLEAR_DE);

    if (ICACHE_MISS) {
        // Check for cancellation due to branch redirect to different block
        if (CLEAR_DE && (miss_block != redirect_block)) {
            printf("[FETCH] -> CANCELLING miss (different block)\n");
            ICACHE_MISS = 0;
            ICACHE_MISS_CYCLES_REMAINING = 0;
            ICACHE_MISS_CANCELLED = 1;
            IF_to_DE_CURRENT = fetched_instruction;  // NOP
            return;
        }
        
        // Check if we need to continue waiting
        if (ICACHE_MISS_CYCLES_REMAINING > 1) {
            ICACHE_MISS_CYCLES_REMAINING--;
            printf("[FETCH] -> MISS stall, cycles remaining: %d\n", ICACHE_MISS_CYCLES_REMAINING);
            IF_to_DE_CURRENT = fetched_instruction;  // NOP
            return;
        }
        
        // Counter is 0 or 1 - miss resolves this cycle
        // Insert block into cache (unless cancelled)
        if (!ICACHE_MISS_CANCELLED) {
            printf("[FETCH] -> MISS resolved, inserting block for PC=0x%lx\n", ICACHE_MISS_PC);
            cache_insert(instruction_cache, ICACHE_MISS_PC);
        }
        ICACHE_MISS = 0;
        ICACHE_MISS_CYCLES_REMAINING = 0;
        // Fall through to fetch
    }

    // Normal cache access
    int icache_hit = cache_check(instruction_cache, fetch_pc);
    printf("[FETCH] -> cache_check(0x%lx) = %s\n", fetch_pc, icache_hit ? "HIT" : "MISS");
    
    if (!icache_hit) {
        printf("[FETCH] -> starting new miss at 0x%lx\n", fetch_pc);
        ICACHE_MISS = 1;
        ICACHE_MISS_PC = fetch_pc;
        ICACHE_MISS_CYCLES_REMAINING = 50;
        ICACHE_MISS_CANCELLED = 0;
        IF_to_DE_CURRENT = fetched_instruction;  // NOP
        return;
    }

    // Cache hit - fetch instruction
    uint32_t raw_inst = mem_read_32(fetch_pc);
    fetched_instruction.raw_instruction = raw_inst;
    fetched_instruction.PC = fetch_pc;
    fetched_instruction.NOP = 0;
    fetched_instruction.INSTRUCTION = UNKNOWN;

    bp_predict(&fetched_instruction);
    printf("[FETCH] -> HIT, fetched from 0x%lx, predicted=0x%lx\n", 
           fetch_pc, fetched_instruction.PREDICTED_PC);

    NEXT_PC = fetched_instruction.PREDICTED_PC;
    IF_to_DE_CURRENT = fetched_instruction;
}