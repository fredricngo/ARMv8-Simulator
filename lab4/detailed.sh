echo -e "go\nrdump" | ./refsim ./inputs/cancel_req.x > ref_final.txt 2>&1
echo -e "go\nrdump" | ./src/sim ./inputs/cancel_req.x 2>&1 | grep -v "^\[FETCH\]" > my_final.txt
diff ref_final.txt my_final.txt