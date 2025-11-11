/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 */

#ifndef _PIPE_H_
#define _PIPE_H_

#include "bp.h"
#include "shell.h"
#include "stdbool.h"
#include <limits.h>

// struct bp_t;
/* Represents the current state of the pipeline. */
typedef struct Pipe_State {
	/* register file state */
	int64_t REGS[ARM_REGS];
	int FLAG_N;        /* flag N */
	int FLAG_Z;        /* flag Z */
	bp_t *bp;

	/* program counter in fetch stage */
	uint64_t PC;

	/* place other information here as necessary */
} Pipe_State;

typedef enum {
    UNKNOWN = 0,
    ADD_EXT,
    ADD_IMM, 
    ADDS_EXT, 
    ADDS_IMM,
    CBNZ, 
    CBZ, 
    AND_SHIFTR,
    ANDS_SHIFTR,
    EOR_SHIFTR, 
    ORR_SHIFTR, 
    LDUR_32,
    LDUR_64,
    LDURB, 
    LDURH, 
    LSL_IMM,
    LSR_IMM, 
    MOVZ,
    STUR_32,
    STUR_64, 
    STURB, 
    STURH, 
    SUB_EXT, 
    SUB_IMM,
    SUBS_EXT, 
    SUBS_IMM, 
    MUL, 
    HLT, 
    CMP_EXT, 
    CMP_IMM, 
    BR,
    B, 
    BEQ, 
    BNE, 
    BGT, 
    BLT, 
    BGE, 
    BLE
} instruction_type_t;

/* Represents an operation travelling through the pipeline. */
typedef struct Pipe_Op {
    uint32_t raw_instruction;
    uint64_t PC;
    int NOP;
    instruction_type_t INSTRUCTION;
    uint64_t RD_REG;
    uint64_t RN_REG;
    uint64_t RN_VAL;
    uint64_t RM_REG;
    uint64_t RM_VAL;
    uint64_t RT_REG;
    uint64_t RT_VAL;
    int64_t  IMM;
    uint32_t SHAM;

    int READ_MEM;
    int WRITE_MEM;
    int LOAD;
    int STORE;

    int UBRANCH;
    int CBRANCH;
    uint64_t BR_TARGET;

    int WRITES_REG;
    int READS_RN;
    int READS_RM;
    int READS_RT;
    int BR_TAKEN; 

    int64_t result;
    int64_t MEM_ADDRESS;
    int64_t MEM_DATA;

    int FLAG_N;
    int FLAG_Z;

	int GHR_XOR_PC;
	uint64_t PREDICTED_PC;
	int BTB_MISS;

} Pipe_Op;


extern int RUN_BIT;

/* global variable -- pipeline state */
extern Pipe_State pipe;

/* called during simulator startup */
void pipe_init();

/* this function calls the others */
void pipe_cycle();

/* each of these functions implements one stage of the pipeline */
void pipe_stage_fetch();
void pipe_stage_decode();
void pipe_stage_execute();
void pipe_stage_mem();
void pipe_stage_wb();

#endif
