/***************************************************************/
/*                                                             */
/*   ARM Instruction Level Simulator                           */
/*                                                             */
/*   CMSC-22200 Computer Architecture                          */
/*   University of Chicago                                     */
/*                                                             */
/***************************************************************/

#include "pipe.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* global pipeline state */
Pipe_State pipe;

int RUN_BIT;

void pipe_init()
{
    memset(&pipe, 0, sizeof(Pipe_State));
    pipe.PC = 0x00400000;
	RUN_BIT = true;
}

void pipe_cycle()
{
	pipe_stage_wb();
	pipe_stage_mem();
	pipe_stage_execute();
	pipe_stage_decode();
	pipe_stage_fetch();
}



void pipe_stage_wb()
{
}

void pipe_stage_mem()
{
}

void pipe_stage_execute()
{
}

void pipe_stage_decode()
{
}

void pipe_stage_fetch()
{
}
