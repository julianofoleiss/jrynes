#This assembly file has a couple of routines for x86_64

.globl memset64

.text

#memset64(void* Dst, uint64_t data, uint64_t n)
#TODO: Rewrite this using REP
memset64:
    pushq %rbp
    pushq %rax
   
    movq %rsp, %rbp        

    movq %rdi, -8(%rbp)    ;rdi = Dst
    movq %rsi, -16(%rbp)   ;rsi = data
    movq %rdx, -24(%rbp)   ;rdx = n

    xorq %rax, %rax
	
set:

    movq %rsi, (%rdi,%rax,8)
    incq %rax

    cmp %rax, %rdx

    jne set

    popq %rax
    popq %rbp

    ret
      
