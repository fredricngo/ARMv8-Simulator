//Fred worked on the instructions labelled F, Kayla worked on the instructions labelled K
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
        uint64_t mask = ~((1ULL << width) - 1); 
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
    instruction_type = UNKNOWN;

    //ADD(EXTENDED) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x458)) {
        instruction_type = ADD_EXT;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        rm = extract_bits(current_instruction, 16, 20);
    }

    //ADD(IMM) - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x91)){
        instruction_type = ADD_IMM;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 10, 21);
        extended_immediate = bit_extension(immediate, 10, 21);
    }

    //ADDS(EXTENDED) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x558)) {
        instruction_type = ADDS_EXT;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        rm = extract_bits(current_instruction, 16, 20);
    }

    //ADDS(IMM) - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xB1)){
        instruction_type = ADDS_IMM; 
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 10, 21);
        extended_immediate = bit_extension(immediate, 10, 21);
    }

    //CBNZ - K
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xB5)) {
        instruction_type = CBNZ;
        rt = extract_bits(current_instruction, 0, 4);
        immediate = extract_bits(current_instruction, 5, 23);
        extended_immediate = bit_extension(immediate, 0, 18);
    }

    //CBZ - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xB4)){
        instruction_type = CBZ;
        rt = extract_bits(current_instruction, 0, 4);
        immediate = extract_bits(current_instruction, 5, 23);
        extended_immediate = bit_extension(immediate, 5, 23);
    }
 
    //AND(SHIFTED REG) - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x8A)){
        instruction_type = AND_SHIFTR;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        rm = extract_bits(current_instruction, 16, 20);
    }

    //ANDS(SHIFTED REGISTER) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x750)) {
        instruction_type = ANDS_SHIFTR;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        rm = extract_bits(current_instruction, 16, 20);
    }
    //EOR(SHIFTED REG) - K
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xCA)) {
        instruction_type = EOR_SHIFTR;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        rm = extract_bits(current_instruction, 16, 20);
    }

    //ORR (SHIFTED REG) - F - orr.s
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xAA)){
        instruction_type = ORR_SHIFTR;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        rm = extract_bits(current_instruction, 16, 20);
    }

    //LDUR (32-AND 64-BIT) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x5C2)) {
        instruction_type = LDUR_32;
        rt = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 12, 20);
        extended_immediate = bit_extension(immediate, 0, 8);
    }

    if (!(extract_bits(current_instruction, 21, 31) ^ 0x7C2)) {
        instruction_type = LDUR_64;
        rt = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 12, 20);
        extended_immediate = bit_extension(immediate, 0, 8);
    }

    //LDURB - F
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x1C2)){
        instruction_type = LDURB;
        rt = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9); 
        immediate = extract_bits(current_instruction, 12, 20); 
        extended_immediate = bit_extension(immediate, 12, 20); 
    }

    //LDURH - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x3C2)) {
        instruction_type = LDURH;
        rt = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 12, 20);
        extended_immediate = bit_extension(immediate, 0, 8);
    }

    //LSL(IMM) - F
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
    if (!(extract_bits(current_instruction, 22, 31) ^ 0x34D) && !(extract_bits(current_instruction, 10, 15) ^ 0x3F)){
        instruction_type = LSR_IMM;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        shift_amount = extract_bits(current_instruction, 16, 21);
    }

    //MOVZ - F
    if (!(extract_bits(current_instruction, 23, 30) ^ 0xA5)){
        instruction_type = MOVZ;
        rd = extract_bits(current_instruction, 0, 4);
        immediate = (uint32_t) extract_bits(current_instruction, 5, 20);
    }

    //STUR (32-AND 64-BIT VAR) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x5C0)) {
        instruction_type = STUR_32;
        rt = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 12, 20);
        extended_immediate = bit_extension(immediate, 0, 8);
    }
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x7C0)) {
        instruction_type = STUR_64;
        rt = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 12, 20);
        extended_immediate = bit_extension(immediate, 0, 8);
    }

    //STURB - F
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x1C0)){
        instruction_type = STURB;
        rt = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9); 
        immediate = extract_bits(current_instruction, 12, 20);
        extended_immediate = bit_extension(immediate, 12, 20);
    }

    //STURH - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x3C0)) {
        instruction_type = STURH;
        rt = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 12, 20);
        extended_immediate = bit_extension(immediate, 0, 8);
    }

    //SUB(IMM) - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xD1)){
        instruction_type = SUB_IMM;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 10, 21);
    }

    //SUB (EXT) - F
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x658)){
        instruction_type = SUB_EXT;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9); 
        rm = extract_bits(current_instruction, 16, 20); 
    }

    //SUBS - K
    if (!(extract_bits(current_instruction, 22, 31) ^ 0x3C4) && (extract_bits(current_instruction, 0, 4) != 31)){
        instruction_type = SUBS_IMM;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 10, 21);
        extended_immediate = bit_extension(immediate, 0, 11);
    }

    //SUBS (EXT) K :)
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x758) && (extract_bits(current_instruction, 0, 4) != 31)) {
        instruction_type = SUBS_EXT;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        rm = extract_bits(current_instruction, 16, 20);
    }

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
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x6B0)){
        instruction_type = BR;
        rn = extract_bits(current_instruction, 5, 9);
    }

    //B - F
    if (!(extract_bits(current_instruction, 26, 31) ^ 0x5)){
        instruction_type = B;
        immediate = extract_bits(current_instruction, 0, 25);
        extended_immediate = bit_extension(immediate, 0, 25);
    }

    //BEQ - K
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x54) && (extract_bits(current_instruction, 0, 3) == 0x0)){
        instruction_type = BEQ;
        immediate = extract_bits(current_instruction, 5, 23);
        extended_immediate = bit_extension(immediate, 0, 18);
    }

    //BNE, BLT, BLE - F
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
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x54) && !(extract_bits(current_instruction, 0, 3) ^ 0xC)){
        instruction_type = BGT;
        immediate = extract_bits(current_instruction, 5, 23);
        extended_immediate = bit_extension(immediate, 0, 18);
    }

    //BGE K
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x54) && !(extract_bits(current_instruction, 0, 3) ^ 0xA)){
        instruction_type = BGE;
        immediate = extract_bits(current_instruction, 5, 23);
        extended_immediate = bit_extension(immediate, 0, 18);
    }
}

void execute()
{
    int64_t result;
    switch(instruction_type) {

        //ADD(EXTENDED)
        case ADD_IMM: 
            result = read_register(rn) + extended_immediate;
            write_register(rd, result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //ADDS(EXTENDED)
        case ADD_EXT:
            result = CURRENT_STATE.REGS[rn] + CURRENT_STATE.REGS[rm];
            write_register(rd, result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //ADDS(IMM) 
        case ADDS_IMM:
            result = read_register(rn) + extended_immediate; 
            NEXT_STATE.FLAG_Z = (result == 0) ? 1 : 0;
            NEXT_STATE.FLAG_N = (result < 0) ? 1 : 0;
            write_register(rd, result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break; 

        //ADDS(EXTENDED)
        case ADDS_EXT:
            result = CURRENT_STATE.REGS[rn] + CURRENT_STATE.REGS[rm];
            write_register(rd, result);
            NEXT_STATE.FLAG_Z = (result == 0);
            NEXT_STATE.FLAG_N = (result >> 63) & 1;
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //AND(SHIFTED REG)
        case AND_SHIFTR:
            result = read_register(rn) & read_register(rm); 
            write_register(rd, result); 
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //ANDS(SHIFTED REGISTER)
        case ANDS_SHIFTR:
            result = CURRENT_STATE.REGS[rn] & CURRENT_STATE.REGS[rm];
            write_register(rd, result);
            NEXT_STATE.FLAG_Z = (result == 0);
            NEXT_STATE.FLAG_N = (result >> 63) & 1;
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //CBNZ
        case CBNZ:
            if (CURRENT_STATE.REGS[rt]) {
                NEXT_STATE.PC = CURRENT_STATE.PC + (extended_immediate << 2);
            } else {
                NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            }
            break;

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
        case EOR_SHIFTR:
            result = CURRENT_STATE.REGS[rn] ^ CURRENT_STATE.REGS[rm];
            write_register(rd, result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //ORR (SHIFTED REG)
        case ORR_SHIFTR:
            result = read_register(rn) | read_register(rm);
            write_register(rd, result); 
            NEXT_STATE.PC = CURRENT_STATE.PC + 4; 
            break; 

        //LDUR (32-AND 64-BIT)
        case LDUR_32:
            result = (int32_t) mem_read_32(CURRENT_STATE.REGS[rn] + extended_immediate);
            write_register(rt, (uint64_t) result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;
        case LDUR_64:
        {
            uint32_t low_word = mem_read_32(CURRENT_STATE.REGS[rn] + extended_immediate);
            uint32_t high_word = mem_read_32(CURRENT_STATE.REGS[rn] + extended_immediate + 4);
            result = ((uint64_t)high_word << 32) | low_word;
            write_register(rt, result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;
        }

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
        case LDURH:
            result = (int16_t) mem_read_32(CURRENT_STATE.REGS[rn] + extended_immediate);
            write_register(rt, (uint64_t) result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //LSL(IMM)
        case LSL_IMM:
        {
            uint32_t actual_shift = (64- immediate) % 64;
            result = read_register(rn) << actual_shift;
            write_register(rd, result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4; 
            break; 
        }

        //LSR (IMM)
        case LSR_IMM:
            result = CURRENT_STATE.REGS[rn] >> shift_amount;
            write_register(rd, result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //MOVZ
        case MOVZ:
            write_register(rd, immediate);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;
        
        //STUR (32-AND 64-BIT VAR)
        case STUR_32:
            mem_write_32(CURRENT_STATE.REGS[rn] + extended_immediate, (uint32_t)(CURRENT_STATE.REGS[rt] & 0xFFFFFFFF));
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;
        case STUR_64:
            mem_write_32(CURRENT_STATE.REGS[rn] + extended_immediate, (uint32_t)(CURRENT_STATE.REGS[rt] & 0xFFFFFFFF));
            mem_write_32(CURRENT_STATE.REGS[rn] + extended_immediate + 4, (uint32_t)(CURRENT_STATE.REGS[rt] >> 32));
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //STURB
        case STURB:
        {
            uint64_t address = read_register(rn) + extended_immediate;

            uint8_t store_byte = read_register(rt) & 0xFF;
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
        case STURH:
            mem_write_32(CURRENT_STATE.REGS[rn] + extended_immediate, (uint32_t)(CURRENT_STATE.REGS[rt] & 0xFFFF));
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

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

        //SUBS (EXT)
        case SUBS_EXT:
            result = CURRENT_STATE.REGS[rn] - CURRENT_STATE.REGS[rm];
            write_register(rd, result);
            NEXT_STATE.FLAG_Z = (result == 0);
            NEXT_STATE.FLAG_N = (result >> 63) & 1;
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //SUBS (IMM)
        case SUBS_IMM:
            result = CURRENT_STATE.REGS[rn] - extended_immediate;
            write_register(rd, result);
            NEXT_STATE.FLAG_Z = (result == 0);
            NEXT_STATE.FLAG_N = (result >> 63) & 1;
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //MUL
        case MUL:
            result = read_register(rn) * read_register(rm);
            write_register(rd, result);
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break; 
        
        case HLT:
            RUN_BIT = 0;
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;
            
        //CMP EXT
        case CMP_EXT:
            result = CURRENT_STATE.REGS[rn] - CURRENT_STATE.REGS[rm];
            NEXT_STATE.FLAG_Z = (result == 0);
            NEXT_STATE.FLAG_N = (result >> 63) & 1;
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;
        
        //CMP IMM
        case CMP_IMM:
            result = CURRENT_STATE.REGS[rn] - extended_immediate;
            NEXT_STATE.FLAG_Z = (result == 0);
            NEXT_STATE.FLAG_N = (result >> 63) & 1;
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //BR
        case BR:
            NEXT_STATE.PC = CURRENT_STATE.REGS[rn];
            break;

        //B
        case B:
            NEXT_STATE.PC = CURRENT_STATE.PC + (extended_immediate << 2);
            break; 

        //BEQ
        case BEQ:
            if (CURRENT_STATE.FLAG_Z) {
                NEXT_STATE.PC = CURRENT_STATE.PC + (extended_immediate << 2);
            } else {
                NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            }
            break;

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
        case BGT:
            if (!CURRENT_STATE.FLAG_Z && (CURRENT_STATE.FLAG_N == 0)) {
                NEXT_STATE.PC = CURRENT_STATE.PC + (extended_immediate << 2);
            } else {
                NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            }
            break;

        //BLT
        case BLT:
        {
            if (CURRENT_STATE.FLAG_N != 0){ 
                NEXT_STATE.PC = CURRENT_STATE.PC + (extended_immediate << 2);
            } else {
                NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            }
            break;
        }
            

        //BGE
        case BGE:
            if (CURRENT_STATE.FLAG_N == 0) {
                NEXT_STATE.PC = CURRENT_STATE.PC + (extended_immediate << 2);
            } else {
                NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            }
            break;
        
        //BLE
        case BLE:
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
