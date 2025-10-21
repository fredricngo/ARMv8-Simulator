/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 */

#ifndef _PIPE_H_
#define _PIPE_H_

#include "shell.h"
#include "stdbool.h"
#include <limits.h>

/* Represents an operation travelling through the pipeline. */
typedef struct Pipe_Op {

	/* place other information here as necessary */
} Pipe_Op;

/* Represents the current state of the pipeline. */
typedef struct Pipe_State {
	/* register file state */
	int64_t REGS[ARM_REGS];
	int FLAG_N;        /* flag N */
	int FLAG_Z;        /* flag Z */

	/* program counter in fetch stage */
	uint64_t PC;

	/* place other information here as necessary */
} Pipe_State;

typedef struct Pipe_Reg{
    uint32_t opcode;
    uint64_t rn;
    uint64_t rm; 
    uint64_t rd;
    uint64_t imm;
    uint32_t sham;

} Pipe_Reg;

Pipe_Reg Pipe_Reg_IFtoDE, Pipe_Reg_DEtoEX, Pipe_Reg_EXtoMEM, Pipe_Reg_MEMtoWB;



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
