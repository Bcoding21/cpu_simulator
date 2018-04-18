.data
buffer:
.space 160
.text
main:
	addi $s1, $0, 128   # Miss
	addi $t1, $0, 64    # Hit
	sll $s1, $s1, 2		# Hit
	sll $t0, $s1, 2     # Hit

	addi $s1, $0, 128   # Miss
	addi $t1, $0, 64    # Hit
	sll $s1, $s1, 2		# Hit
	sll $t0, $s1, 2     # Hit

end:
	jr $ra