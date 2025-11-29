# ARMv8 Pipelined Processor Simulator

A cycle-accurate simulator for a subset of the ARMv8 (AArch64) instruction set, featuring a 5-stage pipeline, branch prediction, and L1 cache simulation. Developed as part of UChicago CMSC-22200 Computer Architecture.

## Overview

This project implements a progressively sophisticated ARM processor simulator across four development phases:

1. **Instruction-Level Simulator** - Functional simulation of ARMv8 instructions
2. **Pipelined Processor** - 5-stage pipeline with data forwarding and hazard detection
3. **Branch Prediction** - Gshare predictor with Branch Target Buffer (BTB)
4. **Cache Simulation** - L1 instruction and data caches with LRU replacement

## Architecture

### Pipeline Stages

```
┌───────┐    ┌───────┐    ┌─────────┐    ┌───────┐    ┌───────────┐
│  IF   │───▶│  ID   │───▶│   EX    │───▶│  MEM  │───▶│    WB     │
│ Fetch │    │Decode │    │ Execute │    │Memory │    │ Writeback │
└───────┘    └───────┘    └─────────┘    └───────┘    └───────────┘
```

### Supported Instructions

| Category | Instructions |
|----------|-------------|
| Arithmetic | `ADD`, `ADDS`, `SUB`, `SUBS`, `MUL`, `CMP` |
| Logical | `AND`, `ANDS`, `EOR`, `ORR` |
| Shift | `LSL`, `LSR` |
| Data Transfer | `LDUR`, `LDURB`, `LDURH`, `STUR`, `STURB`, `STURH`, `MOVZ` |
| Branch | `B`, `BR`, `CBZ`, `CBNZ`, `BEQ`, `BNE`, `BGT`, `BLT`, `BGE`, `BLE` |
| Control | `HLT` |

### Branch Predictor

- **Gshare Predictor**: 8-bit global history register (GHR) XORed with PC bits [9:2]
- **Pattern History Table (PHT)**: 256 entries with 2-bit saturating counters
- **Branch Target Buffer (BTB)**: 1024 entries indexed by PC bits [11:2]

### Cache Configuration

| Parameter | Instruction Cache | Data Cache |
|-----------|------------------|------------|
| Size | 8 KB | 64 KB |
| Associativity | 4-way | 8-way |
| Block Size | 32 bytes | 32 bytes |
| Sets | 64 | 256 |
| Write Policy | — | Write-through, allocate-on-write |
| Miss Penalty | 50 cycles | 50 cycles |
| Replacement | LRU | LRU |

## Building

```bash
cd src
make
```

This produces the `sim` executable.

## Usage

```bash
./sim <program.x> [program2.x ...]
```

### Shell Commands

| Command | Description |
|---------|-------------|
| `go` | Run until HLT |
| `run <n>` | Execute n instructions/cycles |
| `rdump` | Dump registers, flags, and PC |
| `mdump <low> <high>` | Dump memory range |
| `bpdump <pht_lo> <pht_hi> <btb_lo> <btb_hi>` | Dump branch predictor state |
| `input <reg> <val>` | Set register value |
| `?` | Show help |
| `quit` | Exit simulator |

### Example Session

```bash
$ ./sim inputs/fibonacci.x
ARM Simulator

ARM-SIM> run 100
ARM-SIM> rdump
```

## Creating Test Programs

1. Write ARM assembly (`.s` file)
2. Assemble to hex format:
   ```bash
   cd inputs
   ./asm2hex program.s
   ```
3. Run the generated `.x` file in the simulator

### Example Assembly

```asm
// fibonacci.s - Calculate Fibonacci numbers
    movz X0, #0          // F(0) = 0
    movz X1, #1          // F(1) = 1
    movz X2, #10         // Calculate 10 terms
loop:
    add X3, X0, X1       // F(n) = F(n-1) + F(n-2)
    add X0, X1, #0       // Shift values
    add X1, X3, #0
    subs X2, X2, #1      // Decrement counter
    cbnz X2, loop        // Continue if not zero
    HLT #0
```

## Project Structure

```
.
├── src/
│   ├── shell.c, shell.h    # Simulator shell (do not modify)
│   ├── sim.c               # Lab 1: Instruction simulation
│   ├── pipe.c, pipe.h      # Labs 2-4: Pipeline implementation
│   ├── bp.c, bp.h          # Lab 3: Branch predictor
│   └── cache.c, cache.h    # Lab 4: Cache simulation
├── inputs/
│   ├── asm2hex             # Assembly to hex converter
│   └── *.x                 # Test programs
├── ref/
│   └── LEGv8_Reference_Data.pdf
└── aarch64-linux-android-4.9/
    └── ...                 # Cross-compiler toolchain
```

## Implementation Details

### Data Hazard Handling

The simulator implements full data forwarding:
- **MEM → EX forwarding**: When the producing instruction is in MEM stage
- **WB → EX forwarding**: When the producing instruction is in WB stage
- **Load-use stall**: 1-cycle stall when a load is immediately followed by a dependent instruction

### Control Hazard Handling

- Branch target resolution occurs in EX stage
- Pipeline stalls on branch decode until resolution
- With branch prediction: speculative fetch with flush on misprediction

### Memory Model

- Byte-addressable, little-endian
- Three memory regions:
  - **Text**: `0x00400000` (1 MB) - Instructions
  - **Data**: `0x10000000` (1 MB) - Static data
  - **Stack**: `0xFFFFFFFC` (1 MB) - Stack (grows down)

## Architectural State

```c
typedef struct CPU_State {
    uint64_t PC;              // Program counter
    int64_t  REGS[32];        // General-purpose registers (X0-X30, XZR)
    int      FLAG_N;          // Negative flag
    int      FLAG_Z;          // Zero flag
} CPU_State;
```

- **X31** always reads as zero (Zero Register)
- **X30** is the link register (for procedure calls)

## Testing

Compare against the reference simulator:

```bash
./testing.sh
```

Or manually:
```bash
./sim inputs/test.x < commands.txt > my_output.txt
./refsim inputs/test.x < commands.txt > ref_output.txt
diff my_output.txt ref_output.txt
```

## Technical Notes

- All memory accesses must be aligned to their access size
- The simulator uses 32-bit aligned memory access functions internally
- Flag updates only occur for instructions with 'S' suffix (ADDS, SUBS, ANDS, CMP)
- C and V flags are assumed to be 0 for conditional branches

## References

- ARMv8-A Architecture Reference Manual (Part C: A64 Instruction Set)
- LEGv8 Reference Data (simplified ARM subset)

## License

This project was developed for educational purposes as part of UChicago CMSC-22200.

---

*Built with ☕ and many cycles of debugging*
