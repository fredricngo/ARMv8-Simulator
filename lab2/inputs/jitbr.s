.text
mov X1, #target_addr 
mov X5, #0 
mov X6, #0
mov X7, #0
mov X8, #0  
br X1                     
mov X0, #100             
target_addr:
mov X0, #200              
HLT 0
