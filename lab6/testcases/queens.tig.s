.text
.globl tigermain
.type tigermain, @function
tigermain:
 subq $80, %rsp
L32:
 movq $8, %rax
 movq %rax, 72(%rsp)
 leaq 80(%rsp), %rax
 addq $-16, %rax
 movq %rax, 32(%rsp)

 movq $0, %rax
 movq %rax, (%rsp)
 movq 72(%rsp), %rdi
 movq $0, %rsi
 callq initArray
 movq 32(%rsp), %rdi

 movq %rax, (%rdi)
 leaq 80(%rsp), %rax
 addq $-24, %rax
 movq %rax, 24(%rsp)

 movq $0, %rax
 movq %rax, (%rsp)
 movq 72(%rsp), %rdi
 movq $0, %rsi
 callq initArray
 movq 24(%rsp), %rdi

 movq %rax, (%rdi)
 leaq 80(%rsp), %rax
 addq $-32, %rax
 movq %rax, 16(%rsp)

 movq $0, %rax
 movq %rax, (%rsp)
 movq 72(%rsp), %rdi
 movq 72(%rsp), %rax
 addq %rax, %rdi
 subq $1, %rdi
 movq $0, %rsi
 callq initArray
 movq 16(%rsp), %rdi

 movq %rax, (%rdi)
 leaq 80(%rsp), %rax
 addq $-40, %rax
 movq %rax, 8(%rsp)

 movq $0, %rax
 movq %rax, (%rsp)
 movq 72(%rsp), %rdi
 movq 72(%rsp), %rax
 addq %rax, %rdi
 subq $1, %rdi
 movq $0, %rsi
 callq initArray
 movq 8(%rsp), %rdi

 movq %rax, (%rdi)
 leaq 80(%rsp), %rax
 movq %rax, (%rsp)
 movq $0, %rdi
 callq try
 jmp  L31
L31:
 addq $80, %rsp

 retq

.size tigermain, .-tigermain
.globl try
.type try, @function
try:
 subq $32, %rsp
L34:
 movq %rdi, 16(%rsp)

 movq 40(%rsp), %rax
 movq -8(%rax), %rdi
 movq 16(%rsp), %rax

 cmpq %rdi, %rax
 je  L28
L29:
 movq $0, %rax
 movq %rax, 24(%rsp)

 movq 40(%rsp), %rax
 movq -8(%rax), %rax
 subq $1, %rax
 movq %rax, 8(%rsp)

 movq 40(%rsp), %rax
 movq -8(%rax), %rax
 subq $1, %rax
 movq $0, %rdi
 cmpq %rax, %rdi
 jle  L27
L13:
L30:
 movq $0, %rax
 jmp  L33
L28:
 movq 40(%rsp), %rax
 movq %rax, (%rsp)
 callq printboard
 jmp  L30
L27:
 movq $0, %rcx
 movq 40(%rsp), %rax
 movq -16(%rax), %rdi
 movq 24(%rsp), %rax

 movq $8, %rsi
 imulq %rsi
 addq %rax, %rdi
 movq (%rdi), %rax
 cmpq %rcx, %rax
 je  L14
L15:
 movq $0, %rdi
L16:
 movq $0, %rax
 cmpq %rax, %rdi
 jne  L19
L20:
 movq $0, %rdi
L21:
 movq $0, %rax
 cmpq %rax, %rdi
 jne  L24
L25:
 movq 24(%rsp), %rdi

 movq 8(%rsp), %rax

 cmpq %rax, %rdi
 jge  L13
L26:
 movq 24(%rsp), %rax

 addq $1, %rax
 movq %rax, 24(%rsp)

 jmp  L27
L14:
 movq $1, %rdi
 movq $0, %rcx
 movq 40(%rsp), %rax
 movq -32(%rax), %rsi
 movq 24(%rsp), %rax

 movq 16(%rsp), %rdx

 addq %rdx, %rax
 movq $8, %rdx
 imulq %rdx
 addq %rax, %rsi
 movq (%rsi), %rax
 cmpq %rcx, %rax
 je  L17
L18:
 movq $0, %rdi
L17:
 jmp  L16
L19:
 movq $1, %rdi
 movq $0, %rcx
 movq 40(%rsp), %rax
 movq -40(%rax), %rsi
 movq 24(%rsp), %rax

 addq $7, %rax
 movq 16(%rsp), %rdx

 subq %rdx, %rax
 movq $8, %rdx
 imulq %rdx
 addq %rax, %rsi
 movq (%rsi), %rax
 cmpq %rcx, %rax
 je  L22
L23:
 movq $0, %rdi
L22:
 jmp  L21
L24:
 movq 40(%rsp), %rax
 movq -16(%rax), %rdi
 movq 24(%rsp), %rax

 movq $8, %rsi
 imulq %rsi
 addq %rax, %rdi
 movq $1, %rax
 movq %rax, (%rdi)
 movq 40(%rsp), %rax
 movq -32(%rax), %rdi
 movq 24(%rsp), %rax

 movq 16(%rsp), %rsi

 addq %rsi, %rax
 movq $8, %rsi
 imulq %rsi
 addq %rax, %rdi
 movq $1, %rax
 movq %rax, (%rdi)
 movq 40(%rsp), %rax
 movq -40(%rax), %rdi
 movq 24(%rsp), %rax

 addq $7, %rax
 movq 16(%rsp), %rsi

 subq %rsi, %rax
 movq $8, %rsi
 imulq %rsi
 addq %rax, %rdi
 movq $1, %rax
 movq %rax, (%rdi)
 movq 40(%rsp), %rax
 movq -24(%rax), %rdi
 movq 16(%rsp), %rax

 movq $8, %rsi
 imulq %rsi
 addq %rax, %rdi
 movq 24(%rsp), %rax

 movq %rax, (%rdi)
 movq 40(%rsp), %rax
 movq %rax, (%rsp)
 movq 16(%rsp), %rdi

 addq $1, %rdi
 callq try
 movq 40(%rsp), %rax
 movq -16(%rax), %rdi
 movq 24(%rsp), %rax

 movq $8, %rsi
 imulq %rsi
 addq %rax, %rdi
 movq $0, %rax
 movq %rax, (%rdi)
 movq 40(%rsp), %rax
 movq -32(%rax), %rdi
 movq 24(%rsp), %rax

 movq 16(%rsp), %rsi

 addq %rsi, %rax
 movq $8, %rsi
 imulq %rsi
 addq %rax, %rdi
 movq $0, %rax
 movq %rax, (%rdi)
 movq 40(%rsp), %rax
 movq -40(%rax), %rdi
 movq 24(%rsp), %rax

 addq $7, %rax
 movq 16(%rsp), %rsi

 subq %rsi, %rax
 movq $8, %rsi
 imulq %rsi
 addq %rax, %rdi
 movq $0, %rax
 movq %rax, (%rdi)
 jmp  L25
L33:
 addq $32, %rsp

 retq

.size try, .-try
.globl printboard
.type printboard, @function
printboard:
 subq $40, %rsp
L36:
 movq $0, %rax
 movq %rax, 24(%rsp)

 movq 48(%rsp), %rax
 movq -8(%rax), %rax
 subq $1, %rax
 movq %rax, 8(%rsp)

 movq 48(%rsp), %rax
 movq -8(%rax), %rax
 subq $1, %rax
 movq $0, %rdi
 cmpq %rax, %rdi
 jle  L11
L0:
 movq $0, %rax
 movq %rax, (%rsp)
 leaq L12(%rip), %rdi
 callq print
 jmp  L35
L11:
 movq $0, %rax
 movq %rax, 32(%rsp)

 movq 48(%rsp), %rax
 movq -8(%rax), %rax
 subq $1, %rax
 movq %rax, 16(%rsp)

 movq 48(%rsp), %rax
 movq -8(%rax), %rax
 subq $1, %rax
 movq $0, %rdi
 cmpq %rax, %rdi
 jle  L8
L1:
 movq $0, %rax
 movq %rax, (%rsp)
 leaq L9(%rip), %rdi
 callq print
 movq 24(%rsp), %rdi

 movq 8(%rsp), %rax

 cmpq %rax, %rdi
 jge  L0
L10:
 movq 24(%rsp), %rax

 addq $1, %rax
 movq %rax, 24(%rsp)

 jmp  L11
L8:
 movq 48(%rsp), %rax
 movq -24(%rax), %rdi
 movq 24(%rsp), %rax

 movq $8, %rsi
 imulq %rsi
 addq %rax, %rdi
 movq (%rdi), %rdi
 movq 32(%rsp), %rax

 cmpq %rax, %rdi
 je  L4
L5:
 leaq L3(%rip), %rdi
L6:
 movq $0, %rax
 movq %rax, (%rsp)
 callq print
 movq 32(%rsp), %rdi

 movq 16(%rsp), %rax

 cmpq %rax, %rdi
 jge  L1
L7:
 movq 32(%rsp), %rax

 addq $1, %rax
 movq %rax, 32(%rsp)

 jmp  L8
L4:
 leaq L2(%rip), %rdi
 jmp  L6
L35:
 addq $40, %rsp

 retq

.size printboard, .-printboard
.section .rodata
L12:
.long 1
.string "\n"
L9:
.long 1
.string "\n"
L3:
.long 2
.string " ."
L2:
.long 2
.string " O"
