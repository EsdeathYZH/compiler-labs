.text
.globl tigermain
.type tigermain, @function
tigermain:
 subq $32, %rsp
L12:
 movq $16, %rax
 movq %rax, 24(%rsp)
 leaq 32(%rsp), %rax
 addq $-16, %rax
 movq %rax, 8(%rsp)

 movq $0, %rax
 movq %rax, (%rsp)
 movq 24(%rsp), %rdi
 movq $0, %rsi
 callq initArray
 movq 8(%rsp), %rdi

 movq %rax, (%rdi)
 leaq 32(%rsp), %rax
 movq %rax, (%rsp)
 callq try
 jmp  L11
L11:
 addq $32, %rsp

 retq

.size tigermain, .-tigermain
.globl try
.type try, @function
try:
 subq $8, %rsp
L14:
 movq 16(%rsp), %rax
 movq %rax, (%rsp)
 callq init
 movq 16(%rsp), %rax
 movq %rax, (%rsp)
 movq $0, %rdi
 movq 16(%rsp), %rax
 movq -8(%rax), %rsi
 subq $1, %rsi
 movq $7, %rdx
 callq bsearch
 movq %rax, %rdi
 leaq 8(%rsp), %rax
 movq %rax, (%rsp)
 callq printi
 leaq 8(%rsp), %rax
 movq %rax, (%rsp)
 leaq L10(%rip), %rdi
 callq print
 jmp  L13
L13:
 addq $8, %rsp

 retq

.size try, .-try
.globl bsearch
.type bsearch, @function
bsearch:
 subq $40, %rsp
L16:
 movq %rdi, 24(%rsp)

 movq %rsi, 8(%rsp)

 movq %rdx, 16(%rsp)

 movq 24(%rsp), %rdi

 movq 8(%rsp), %rax

 cmpq %rax, %rdi
 je  L7
L8:
 movq 24(%rsp), %rax

 movq 8(%rsp), %rdi

 addq %rdi, %rax
 cqto
 movq $2, %rdi
 idivq %rdi
 movq %rax, 32(%rsp)
 movq 48(%rsp), %rax
 movq -16(%rax), %rdi
 movq 32(%rsp), %rax
 movq $8, %rsi
 imulq %rsi
 addq %rax, %rdi
 movq (%rdi), %rdi
 movq 16(%rsp), %rax

 cmpq %rax, %rdi
 jl  L4
L5:
 movq 48(%rsp), %rax
 movq %rax, (%rsp)
 movq 24(%rsp), %rdi

 movq 32(%rsp), %rsi
 movq 16(%rsp), %rdx

 callq bsearch
L6:
L9:
 jmp  L15
L7:
 movq 24(%rsp), %rax

 jmp  L9
L4:
 movq 48(%rsp), %rax
 movq %rax, (%rsp)
 movq 32(%rsp), %rdi
 addq $1, %rdi
 movq 8(%rsp), %rsi

 movq 16(%rsp), %rdx

 callq bsearch
 jmp  L6
L15:
 addq $40, %rsp

 retq

.size bsearch, .-bsearch
.globl init
.type init, @function
init:
 subq $24, %rsp
L18:
 movq $0, %rax
 movq %rax, 16(%rsp)

 movq 32(%rsp), %rax
 movq -8(%rax), %rax
 subq $1, %rax
 movq %rax, 8(%rsp)

 movq 32(%rsp), %rax
 movq -8(%rax), %rax
 subq $1, %rax
 movq $0, %rdi
 cmpq %rax, %rdi
 jle  L3
L1:
 movq $0, %rax
 jmp  L17
L3:
 movq 32(%rsp), %rax
 movq -16(%rax), %rdi
 movq 16(%rsp), %rax

 movq $8, %rsi
 imulq %rsi
 addq %rax, %rdi
 movq 16(%rsp), %rax

 movq $2, %rsi
 imulq %rsi
 addq $1, %rax
 movq %rax, (%rdi)
 movq 32(%rsp), %rax
 movq %rax, (%rsp)
 callq nop
 movq 16(%rsp), %rdi

 movq 8(%rsp), %rax

 cmpq %rax, %rdi
 jge  L1
L2:
 movq 16(%rsp), %rax

 addq $1, %rax
 movq %rax, 16(%rsp)

 jmp  L3
L17:
 addq $24, %rsp

 retq

.size init, .-init
.globl nop
.type nop, @function
nop:
 subq $8, %rsp
L20:
 leaq 8(%rsp), %rax
 movq %rax, (%rsp)
 leaq L0(%rip), %rdi
 callq print
 jmp  L19
L19:
 addq $8, %rsp

 retq

.size nop, .-nop
.section .rodata
L10:
.long 1
.string "\n"
L0:
.long 0
.string ""
