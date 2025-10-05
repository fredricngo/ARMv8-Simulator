#!/bin/bash
LABN=lab1
CORRECT_TESTS=0
NUM_TESTS=36
declare -a inputfilelist=("add.x" "addi.x" "adds.x" "addis.x" "cbnz.x" "cbz.x" "and.x" "ands.x" 
	"eor.x" "orr.x" "ldur.x" "ldurb.x" "ldurh.x" "lsli.x" "lsri.x" "movz.x" "stur.x" "sturb.x" 
	"sturh.x" "sub.x" "subi.x" "subs.x" "subsi.x" "mul.x" "cmp.x" "br.x" "b.x" "beq.x" "bne.x" 
	"bgt.x" "blt.x" "bge.x" "ble.x" "countdigits.x" "fibonacci.x" "load_store.x")
echo "lab1-tests.txt"  >> "lab1-tests.txt"
echo "---------"  >> "lab1-tests.txt"

if [ ! -f src/sim ]; then
    echo "src/sim does not exist (or you called this script from the wrong directory...). Compile your code first, and ensure you are running this script from the directory where it lives."
	exit 1
fi

mkdir -p test
./cleaning.sh

for inputfile in "${inputfilelist[@]}";
do
	echo "$inputfile"
	value=1
	i=1
	mismatch=0
	cycle=1
	
	while [ $i -eq $value ]
		do	# change "run $cycle" to "go" to run the entire program without stepping through cycle-by-cycle
			timeout 5 echo "
			input 11 2\n 
			input 12 6\n 
			input 20 400008\n 
			input 1 128\n 
			input 10 10000000\n  
			input 2 2\n
			input 3 3\n
			input 4 4\n
			input 5 5\n
			input 6 6\n
			input 7 7\n
			input 8 8\n
			input 9 9\n
			input 13 13\n
			input 14 14\n
			input 28 28\n
			run $cycle\n rdump\n mdump 0x10000000 0x100000ff" | timeout 1 ./refsim inputs/${inputfile} > test/reference_state.txt
			cd src
			timeout 5 echo "
			input 11 2\n 
			input 12 6\n 
			input 20 400008\n 
			input 1 128\n 
			input 10 10000000\n 
			input 2 2\n
			input 3 3\n
			input 4 4\n
			input 5 5\n
			input 6 6\n
			input 7 7\n
			input 8 8\n
			input 9 9\n
			input 13 13\n
			input 14 14\n
			input 28 28\n	
			run $cycle\n rdump\n mdump 0x10000000 0x100000ff" | timeout 1 ./sim ../inputs/${inputfile} > ../test/actual_state.txt
			cd .. # we are comparing the dumpsim files
			echo "------------------cycle $cycle---------------" >> test/diff_${inputfile}.txt
			value2=$(echo "$value2" | grep -r 'halted' test/reference_state.txt)
			value3=$(echo "$value3" | grep -r 'halted' test/actual_state.txt)
			if [[ -z "${value2// }" ]] && [[ -z "${value3// }" ]] # if length of string is 0
				then
					cycle=$((cycle+1))
				else		
					i=2
			fi
			diff ./dumpsim ./src/dumpsim >> test/diff_final_${inputfile}.txt
		done
		if [ -s test/diff_final_${inputfile}.txt ] # if the file is not empty
		then
			if [ ! -f "$LABN-tests.txt" ]; then
				touch "$LABN-tests.txt"
		   fi
		   echo "Incorrect test: ${inputfile}" >> "$LABN-tests.txt"
		else
			let "CORRECT_TESTS++" # diff file is empty, indicating that the test was passed
		fi
done
echo "Correct tests: $CORRECT_TESTS"  >> "$LABN-tests.txt"
NUM_INCORRECT=$((NUM_TESTS - CORRECT_TESTS))
echo "Incorrect tests: $NUM_INCORRECT"  >> "$LABN-tests.txt"
echo "Correct tests: $CORRECT_TESTS / $NUM_TESTS"