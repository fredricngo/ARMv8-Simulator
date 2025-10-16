#include <stdio.h>
#include "shell.h"

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
    LDUR,
    LDURB, 
    LDURH, 
    LSL_IMM,
    LSR_IMM, 
    MOVZ,
    STUR, 
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

instruction_type_t instruction_type;
uint32_t current_instruction;
int64_t rd, rn, rm;
int64_t rt; 
int32_t immediate;
int64_t extended_immediate; 
uint32_t shift_amount;

uint32_t extract_bits(uint32_t instruction, int start, int end){
    /* Given an instruction type, returns a section from start: end (inclusive)*/

    int width = end - start + 1;
    uint32_t mask = (1U << width) - 1;
    return (instruction >> start) & mask;
}

int64_t bit_extension(int32_t immediate, int start, int end){
    /*
    This function receives an immediate (int32_t) and: 
    (1) Checks if the MSB is set to 1 (signed int)
    (2) if (1), extends to a uint64_t for arithmetic.
    */

    int width = end - start + 1;


    
    if ((immediate >> (width - 1)) & 1) {
        //MSB is set to 1, so we perform bit extension with upper bits as 1s
        uint64_t mask = ~((1ULL << width) - 1); //1ULL makes 1 64 bits long

        // printf("  mask=0x%x, result=0x%llx\n", mask, (int64_t)(immediate | mask));

        return (int64_t) (immediate | mask);
    } else {
        return (int64_t) immediate; 
    }
}

int64_t read_register(int reg_num){
    if (reg_num == 31){
        return 0;
    }
    return CURRENT_STATE.REGS[reg_num];
}

void write_register(int reg_num, int64_t value){
    if (reg_num != 31){
        NEXT_STATE.REGS[reg_num] = value; 
    }
}


void fetch()
{
    //Given the instruction address, we retrieve from memory using mem_read_32
    current_instruction = mem_read_32(CURRENT_STATE.PC);
}

void decode()
{
    //Extract the operands using extract_bits, need to check opcodes and specific bit orientations
    //Need to update the registers
    instruction_type = UNKNOWN;

    //ADD(EXTENDED) - K

    //ADD(IMM) = addi.s
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x91)){
        instruction_type = ADD_IMM;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 10, 21);
        extended_immediate = bit_extension(immediate, 10, 21);
    }

    //ADDS(EXTENDED) - K

    //ADDS(IMM) - F - addis.s
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xB1)){
        instruction_type = ADDS_IMM; 
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 10, 21);
        extended_immediate = bit_extension(immediate, 10, 21);
    }

    //CBNZ - K

    //CBZ - F - cbz.s
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xB4)){
        instruction_type = CBZ;
        rt = extract_bits(current_instruction, 0, 4);
        immediate = extract_bits(current_instruction, 5, 23);
        extended_immediate = bit_extension(immediate, 5, 23);
    }
 
    //AND(SHIFTED REG) - and.s
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x8A)){
        instruction_type = AND_SHIFTR;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        rm = extract_bits(current_instruction, 16, 20);
    }

    //ANDS(SHIFTED REGISTER) - K

    //EOR(SHIFTED REG) - K

    //ORR (SHIFTED REG) - F - orr.s
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xAA)){
        instruction_type = ORR_SHIFTR;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        rm = extract_bits(current_instruction, 16, 20);
    }

    //LDUR (32-AND 64-BIT) - K

    //LDURB - F - ldurb.s
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x1C2)){
        instruction_type = LDURB;
        rt = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9); 
        immediate = extract_bits(current_instruction, 12, 20); 
        extended_immediate = bit_extension(immediate, 12, 20); 
    }

    //LDURH - K

    //LSL(IMM) - F - lsli.s
    if ((!(extract_bits(current_instruction, 22, 31) ^ 0x34D)) && ((extract_bits(current_instruction, 10, 15) != 0x3F))){
        uint32_t immr = extract_bits(current_instruction, 16, 21);  // Check this field
        uint32_t imms = extract_bits(current_instruction, 10, 15);

 
        instruction_type = LSL_IMM;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9); 
        immediate = extract_bits(current_instruction, 16, 21);
        extended_immediate = extract_bits(immediate, 16, 21);
    }
    
    //LSR (IMM) - K

    //MOVZ: !!Shift not required!! - movz.s
    if (!(extract_bits(current_instruction, 23, 30) ^ 0xA5)){
        instruction_type = MOVZ;
        rd = extract_bits(current_instruction, 0, 4);
        immediate = (uint32_t) extract_bits(current_instruction, 5, 20);
    }

    //STUR (32-AND 64-BIT VAR) - K

    //STURB - F
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x1C0)){
        instruction_type = STURB;
        rt = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9); 
        immediate = extract_bits(current_instruction, 12, 20);
        extended_immediate = bit_extension(immediate, 12, 20);
    }


    //STURH - K

    //SUB(IMM) - F - subi.s
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xD1)){
        instruction_type = SUB_IMM;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 10, 21);
    }

    //SUB (EXT) - F - sub.s
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x658)){
        instruction_type = SUB_EXT;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9); 
        rm = extract_bits(current_instruction, 16, 20); 
    }
    //SUBS - K

    //MUL - F - mul.s
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x4D8)){
        instruction_type = MUL;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9); 
        rm = extract_bits(current_instruction, 16, 20); 
    }

    //HLT:
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x6a2)){
        instruction_type = HLT;
    }
    //CMP - K
    //CMP (EXT) K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x758)  && (extract_bits(current_instruction, 0, 4) == 31)) {
        instruction_type = CMP_EXT;
        rn = extract_bits(current_instruction, 5, 9);
        rm = extract_bits(current_instruction, 16, 20);
    }
    
    // CMP (IMM) K
    if (!(extract_bits(current_instruction, 22, 31) ^ 0x3c4) && (extract_bits(current_instruction, 0, 4) == 31)){
        instruction_type = CMP_IMM;
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 10, 21);
        extended_immediate = bit_extension(immediate, 0, 11);
    }

    //BR - K

    //B - F - b.s
    if (!(extract_bits(current_instruction, 26, 31) ^ 0x5)){
        instruction_type = B;
        immediate = extract_bits(current_instruction, 0, 25);
        extended_immediate = bit_extension(immediate, 0, 25);
    }


    //BEQ - K

    //BNE - F - bne.s
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x54)){
        unsigned int cond_code = extract_bits(current_instruction, 0, 3);
        immediate = extract_bits(current_instruction, 5, 23);
        extended_immediate = bit_extension(immediate, 5, 23);

        switch (cond_code){
            case 0x1:
                instruction_type = BNE;
                break;
            case 0xB:
                instruction_type = BLT;
                break;
            case 0xD:
                instruction_type = BLE;
                break;
        }
    }

    //BGT - K

    //BLT - F - blt.s

    //BGE - K
    
    //BLE - F - ble.s
}

void execute()
{
    int64_t result;
    //Checks the global instruction parameters, then performs the relevant operations
    switch(instruction_type) {

        //ADD(EXTENDED)

        case ADD_IMM: 
            result = read_register(rn) + extended_immediate;
            write_register(rd, result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //ADDS(EXTENDED)

        //ADDS(IMM) 
        case ADDS_IMM:
            result = read_register(rn) + extended_immediate; 
            NEXT_STATE.FLAG_Z = (result == 0) ? 1 : 0;
            NEXT_STATE.FLAG_N = (result < 0) ? 1 : 0;
            write_register(rd, result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break; 

        //AND(SHIFTED REG)
        case AND_SHIFTR:
            result = read_register(rn) & read_register(rm); 
            write_register(rd, result); 
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //ANDS(SHIFTED REGISTER)

        //CBNZ

        //CBZ
        case CBZ:

            if (read_register(rt) == 0) {
                int64_t byte_offset = extended_immediate << 2; 
                NEXT_STATE.PC = (uint64_t) ((int64_t) CURRENT_STATE.PC + byte_offset);
            }
            else {
                NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            }
            break; 

        //EOR(SHIFTED REG)

        //ORR (SHIFTED REG)
        case ORR_SHIFTR:
            result = read_register(rn) | read_register(rm);
            write_register(rd, result); 
            NEXT_STATE.PC = CURRENT_STATE.PC + 4; 
            break; 

        //LDUR (32-AND 64-BIT)

        //LDURB 
        case LDURB:
        {
            uint64_t address = read_register(rn) + extended_immediate;

            uint32_t word = mem_read_32(address & ~3);
            
            int byte_offset = address & 3;
            uint8_t byte_data = (word >> (byte_offset * 8)) & 0xFF;

            write_register(rt, (uint64_t)byte_data);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break; 
        }
            

        //LDURH 

        //LSL(IMM)
        case LSL_IMM:
        {
            uint32_t actual_shift = (64- immediate) % 64;
            result = read_register(rn) << actual_shift;
            write_register(rd, result); //might be an issue with casting the shifted to a signed int?
            NEXT_STATE.PC = CURRENT_STATE.PC + 4; 
            break; 
        }

        //LSR (IMM)

        case MOVZ: // haven't implemented shift
            //we update NEW_STATE registers
            //We move the immediate into the register
            //only need to implement 64-bit variant, no shift needed
            write_register(rd, immediate);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;
        
        //STUR (32-AND 64-BIT VAR)

        //STURB
        case STURB:
        {
            uint64_t address = read_register(rn) + extended_immediate; //calculates address from base register and imm offset

            uint8_t store_byte = read_register(rt) & 0xFF; //grabs one byte from the other register

            uint32_t aligned_addr = address & ~3;
            int byte_offset = address & 3;

            uint32_t current_word = mem_read_32(aligned_addr);

            uint32_t mask = ~(0xFF << (byte_offset * 8));
            uint32_t new_word = (current_word & mask)  | (store_byte << (byte_offset *8));

            mem_write_32(aligned_addr, new_word); 
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break; 
        }

        //STURH

        //SUB(IMM)
        case SUB_IMM:
            result = read_register(rn) - (int64_t) immediate;
            write_register(rd, result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //SUB (EXT)
        case SUB_EXT:
            result = read_register(rn) - read_register(rm);
            write_register(rd, result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break; 

        //SUBS

        //MUL
        case MUL:
            result = read_register(rn) * read_register(rm);
            write_register(rd, result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break; 
        
        case HLT:
            //if HLT, we just set RUN_BIT to 0
            RUN_BIT = 0;
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;
            
        
        //CMP
        //CMP ext
        case CMP_EXT:
            result = CURRENT_STATE.REGS[rn] - CURRENT_STATE.REGS[rm];
            NEXT_STATE.FLAG_Z = (result == 0);
            NEXT_STATE.FLAG_N = (result >> 63) & 1;
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;
        
        //CMP imm
        case CMP_IMM:
            result = CURRENT_STATE.REGS[rn] - extended_immediate;
            NEXT_STATE.FLAG_Z = (result == 0);
            NEXT_STATE.FLAG_N = (result >> 63) & 1;
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //BR

        //B
        case B:
            NEXT_STATE.PC = CURRENT_STATE.PC + (extended_immediate << 2);
            break; 

        //BEQ

        //BNE
        case BNE:
        {
            if (CURRENT_STATE.FLAG_Z == 0) {
                NEXT_STATE.PC = CURRENT_STATE.PC + (extended_immediate << 2);
            } else {
                NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            }
            break; 
        }

        //BGT

        //BLT

        case BLT: //V flag is assumed to be 0
        {
            if (CURRENT_STATE.FLAG_N != 0){ 
                NEXT_STATE.PC = CURRENT_STATE.PC + (extended_immediate << 2);
            } else {
                NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            }
            break;
        }
        
            

        //BGE
        
        //BLE
        case BLE: //v flag assumed 0 
        {
            if (!((CURRENT_STATE.FLAG_Z == 0) && (CURRENT_STATE.FLAG_N == 0))){
                NEXT_STATE.PC = CURRENT_STATE.PC + (extended_immediate << 2);
            } else {
                NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            }
            break;
        }
            
    }
    
}

void process_instruction()
{
    /* execute one instruction here. You should use CURRENT_STATE and modify
     * values in NEXT_STATE. You can call mem_read_32() and mem_write_32() to
     * access memory. */
    fetch();
    decode();
    execute();

}
