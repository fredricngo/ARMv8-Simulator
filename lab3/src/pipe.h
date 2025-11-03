/***************************************************************/
/*                                                             */
/*   ARM Instruction Level Simulator                           */
/*                                                             */
/*   CMSC-22200 Computer Architecture                          */
/*   University of Chicago                                     */
/*                                                             */
/***************************************************************/

#ifndef _PIPE_H_
#define _PIPE_H_

#include "bp.h"
#include "shell.h"
#include "stdbool.h"
#include <limits.h>

/* Represents the current state of the pipeline. */
typedef struct Pipe_State {
	/* register file state */
	int64_t REGS[ARM_REGS];
	int FLAG_N;        /* flag N */
	int FLAG_Z;        /* flag Z */

	/* program counter in fetch stage */
	uint64_t PC;

	/* place other information here as necessary */

	/* branch predictor */
    bp_t *bp;
} Pipe_State;

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
