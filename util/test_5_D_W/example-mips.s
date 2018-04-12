.data
buffer:
.space 160
.text
main:
	add $t0, $0, $0
	addi $t1, $t0, 23456
	addi $t1, $t1, 32164
	beq $t1, $t0, end
	addi $s1, $s1, 1
	sll $s1, $s1 28
	add $s1, $s1, 4
	sw $t1, 0($s1)
	lw $t3, 0($s1)
	lw $t4, 0($s1)
	addi $t3, $0, 7
	sw $t3, 0($s1)
	lw $t5, 0($s1)
	ori $s6, $t2, 164
	xori $s7, $t2, 164
	sll $0, $0, 0
end:
	jr $ra
