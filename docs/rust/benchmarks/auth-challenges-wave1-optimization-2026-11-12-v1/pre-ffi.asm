	.def	@feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
@feat.00 = 0
	.att_syntax
	.file	"auth_challenge_ffi.32c6d215f83907e0-cgu.0"
	.def	rustium_auth_decode;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",one_only,rustium_auth_decode,unique,0
	.globl	rustium_auth_decode
	.p2align	4
rustium_auth_decode:
.seh_proc rustium_auth_decode
	pushq	%r15
	.seh_pushreg %r15
	pushq	%r14
	.seh_pushreg %r14
	pushq	%r13
	.seh_pushreg %r13
	pushq	%r12
	.seh_pushreg %r12
	pushq	%rsi
	.seh_pushreg %rsi
	pushq	%rdi
	.seh_pushreg %rdi
	pushq	%rbp
	.seh_pushreg %rbp
	pushq	%rbx
	.seh_pushreg %rbx
	subq	$72, %rsp
	.seh_stackalloc 72
	.seh_endprologue
	testq	%rdx, %rdx
	je	.LBB0_1
	movq	%rcx, %rbx
	cmpq	$4, %rdx
	setb	%al
	testb	$3, %dl
	setne	%cl
	movq	$-1, %rsi
	orb	%al, %cl
	jne	.LBB0_2
	movq	%r8, %r12
	cmpb	$61, -1(%rbx,%rdx)
	movq	%rdx, %r14
	jne	.LBB0_6
	leaq	-1(%rdx), %r14
	leaq	-2(%rdx), %rax
	cmpb	$61, -2(%rbx,%rdx)
	cmoveq	%rax, %r14
.LBB0_6:
	movq	%rdx, %rdi
	movq	%r14, %rax
	shrq	$2, %rax
	leaq	(%rax,%rax,2), %r15
	addq	$2, %r15
	callq	_RNvCs6d6XRwTwhVh_7___rustc35___rust_no_alloc_shim_is_unstable_v2
	movl	$1, %edx
	movq	%r15, %rcx
	callq	_RNvCs6d6XRwTwhVh_7___rustc12___rust_alloc
	testq	%rax, %rax
	je	.LBB0_57
	movq	%r15, 40(%rsp)
	movq	%rax, 48(%rsp)
	movq	$0, 56(%rsp)
	cmpq	%rdi, %r14
	ja	.LBB0_58
	movq	%r12, 64(%rsp)
	xorl	%r15d, %r15d
	leaq	40(%rsp), %r12
	jmp	.LBB0_9
	.p2align	4
.LBB0_31:
	movq	%rax, %r15
.LBB0_32:
	addq	%r13, %rbx
	subq	%r13, %r14
	je	.LBB0_33
.LBB0_9:
	cmpq	$4, %r14
	movl	$4, %r13d
	cmovbq	%r14, %r13
	movzbl	(%rbx), %eax
	leal	-65(%rax), %edi
	cmpb	$26, %dil
	jb	.LBB0_15
	leal	-97(%rax), %ecx
	cmpb	$26, %cl
	jae	.LBB0_11
	addb	$-71, %al
	movl	%eax, %edi
	jmp	.LBB0_15
	.p2align	4
.LBB0_11:
	leal	-48(%rax), %ecx
	cmpb	$10, %cl
	jae	.LBB0_12
	addb	$4, %al
	movl	%eax, %edi
	jmp	.LBB0_15
.LBB0_12:
	movzbl	%al, %eax
	movb	$62, %dil
	cmpl	$43, %eax
	je	.LBB0_15
	cmpl	$47, %eax
	jne	.LBB0_21
	movb	$63, %dil
	.p2align	4
.LBB0_15:
	cmpq	$1, %r14
	je	.LBB0_25
	movzbl	1(%rbx), %eax
	leal	-65(%rax), %ebp
	cmpb	$26, %bpl
	jb	.LBB0_28
	leal	-97(%rax), %ecx
	cmpb	$26, %cl
	jae	.LBB0_18
	addb	$-71, %al
	jmp	.LBB0_27
	.p2align	4
.LBB0_18:
	leal	-48(%rax), %ecx
	cmpb	$10, %cl
	jae	.LBB0_19
	addb	$4, %al
.LBB0_27:
	movl	%eax, %ebp
.LBB0_28:
	cmpq	40(%rsp), %r15
	jne	.LBB0_30
	movq	%r12, %rcx
	callq	_RNvMs7_NtCs8tsiLsQ9XHg_5alloc7raw_vecINtB5_6RawVechE8grow_oneB7_
.LBB0_30:
	shlb	$2, %dil
	movl	%ebp, %eax
	shrb	$4, %al
	orb	%dil, %al
	movq	48(%rsp), %rcx
	movb	%al, (%rcx,%r15)
	leaq	1(%r15), %rax
	movq	%rax, 56(%rsp)
	cmpq	$2, %r14
	jbe	.LBB0_31
	movzbl	2(%rbx), %ecx
	leal	-65(%rcx), %edi
	cmpb	$26, %dil
	jb	.LBB0_45
	leal	-97(%rcx), %edx
	cmpb	$26, %dl
	jae	.LBB0_41
	addb	$-71, %cl
	movl	%ecx, %edi
	jmp	.LBB0_45
.LBB0_19:
	movzbl	%al, %eax
	movb	$62, %bpl
	cmpl	$43, %eax
	je	.LBB0_28
	cmpl	$47, %eax
	jne	.LBB0_21
	movb	$63, %bpl
	jmp	.LBB0_28
.LBB0_41:
	leal	-48(%rcx), %edx
	cmpb	$10, %dl
	jae	.LBB0_42
	addb	$4, %cl
	movl	%ecx, %edi
	jmp	.LBB0_45
.LBB0_42:
	movzbl	%cl, %ecx
	movb	$62, %dil
	cmpl	$43, %ecx
	je	.LBB0_45
	cmpl	$47, %ecx
	jne	.LBB0_21
	movb	$63, %dil
	.p2align	4
.LBB0_45:
	cmpq	40(%rsp), %rax
	jne	.LBB0_47
	movq	%r12, %rcx
	callq	_RNvMs7_NtCs8tsiLsQ9XHg_5alloc7raw_vecINtB5_6RawVechE8grow_oneB7_
.LBB0_47:
	shlb	$4, %bpl
	movl	%edi, %eax
	shrb	$2, %al
	orb	%bpl, %al
	movq	48(%rsp), %rcx
	movb	%al, 1(%rcx,%r15)
	leaq	2(%r15), %rax
	movq	%rax, 56(%rsp)
	cmpq	$3, %r14
	je	.LBB0_31
	movzbl	3(%rbx), %ecx
	leal	-65(%rcx), %ebp
	cmpb	$26, %bpl
	jb	.LBB0_54
	leal	-97(%rcx), %edx
	cmpb	$26, %dl
	jae	.LBB0_50
	addb	$-71, %cl
	movl	%ecx, %ebp
	jmp	.LBB0_54
.LBB0_50:
	leal	-48(%rcx), %edx
	cmpb	$10, %dl
	jae	.LBB0_51
	addb	$4, %cl
	movl	%ecx, %ebp
	jmp	.LBB0_54
.LBB0_51:
	movzbl	%cl, %ecx
	movb	$62, %bpl
	cmpl	$43, %ecx
	je	.LBB0_54
	cmpl	$47, %ecx
	jne	.LBB0_21
	movb	$63, %bpl
	.p2align	4
.LBB0_54:
	cmpq	40(%rsp), %rax
	jne	.LBB0_56
	movq	%r12, %rcx
	callq	_RNvMs7_NtCs8tsiLsQ9XHg_5alloc7raw_vecINtB5_6RawVechE8grow_oneB7_
.LBB0_56:
	shlb	$6, %dil
	orb	%dil, %bpl
	movq	48(%rsp), %rax
	movb	%bpl, 2(%rax,%r15)
	addq	$3, %r15
	movq	%r15, 56(%rsp)
	jmp	.LBB0_32
.LBB0_1:
	xorl	%esi, %esi
	jmp	.LBB0_2
.LBB0_33:
	movq	40(%rsp), %rbx
	cmpq	$-1, %rbx
	je	.LBB0_2
	movq	48(%rsp), %rsi
	testq	%r15, %r15
	je	.LBB0_36
	movq	64(%rsp), %rcx
	movq	%rsi, %rdx
	movq	%r15, %r8
	callq	memcpy
.LBB0_36:
	testq	%rbx, %rbx
	je	.LBB0_38
	movl	$1, %r8d
	movq	%rsi, %rcx
	movq	%rbx, %rdx
	callq	_RNvCs6d6XRwTwhVh_7___rustc14___rust_dealloc
.LBB0_38:
	movq	%r15, %rsi
.LBB0_2:
	movq	%rsi, %rax
	.seh_startepilogue
	addq	$72, %rsp
	popq	%rbx
	popq	%rbp
	popq	%rdi
	popq	%rsi
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	.seh_endepilogue
	retq
.LBB0_21:
	movq	40(%rsp), %rdx
	testq	%rdx, %rdx
	je	.LBB0_2
	movq	48(%rsp), %rcx
	movl	$1, %r8d
	callq	_RNvCs6d6XRwTwhVh_7___rustc14___rust_dealloc
	jmp	.LBB0_2
.LBB0_58:
	leaq	anon.fde56726e783d76ca05f96b9a8b0bccb.2(%rip), %r9
	xorl	%ecx, %ecx
	movq	%rdi, %r8
	movq	%r14, %rdx
	callq	_RNvNtNtCs9l4wcBNbII5_4core5slice5index16slice_index_fail
	ud2
.LBB0_25:
	leaq	anon.fde56726e783d76ca05f96b9a8b0bccb.1(%rip), %r8
	movl	$1, %ecx
	movl	$1, %edx
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_bounds_check
	ud2
.LBB0_57:
	movl	$1, %ecx
	movq	%r15, %rdx
	callq	_RNvNtCs8tsiLsQ9XHg_5alloc7raw_vec12handle_error
	ud2
	.seh_endproc

	.def	rustium_auth_encoded_len;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",one_only,rustium_auth_encoded_len,unique,1
	.globl	rustium_auth_encoded_len
	.p2align	4
rustium_auth_encoded_len:
.seh_proc rustium_auth_encoded_len
	subq	$40, %rsp
	.seh_stackalloc 40
	.seh_endprologue
	movq	%rdx, %r8
	andq	$-4, %r8
	testb	$3, %dl
	je	.LBB1_10
	movq	%rdx, %rax
	subq	$1, %rax
	jb	.LBB1_15
	cmpb	$61, (%rcx,%rax)
	jne	.LBB1_11
	testb	$3, %al
	je	.LBB1_10
	leaq	-2(%rdx), %r9
	cmpq	$1, %rdx
	je	.LBB1_14
	cmpb	$61, (%rcx,%r9)
	jne	.LBB1_9
	testb	$3, %r9b
	je	.LBB1_10
	movq	%rdx, %rax
	subq	$3, %rax
	jb	.LBB1_15
	cmpb	$61, (%rcx,%rax)
	cmoveq	%r8, %r9
	movq	%r9, %rax
.LBB1_9:
	.seh_startepilogue
	addq	$40, %rsp
	.seh_endepilogue
	retq
.LBB1_10:
	movq	%r8, %rax
	.seh_startepilogue
	addq	$40, %rsp
	.seh_endepilogue
	retq
.LBB1_11:
	movq	%rdx, %rax
	.seh_startepilogue
	addq	$40, %rsp
	.seh_endepilogue
	retq
.LBB1_14:
	movq	%r9, %rax
.LBB1_15:
	leaq	anon.fde56726e783d76ca05f96b9a8b0bccb.5(%rip), %r8
	movq	%rax, %rcx
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_bounds_check
	ud2
	.seh_endproc

	.def	rustium_auth_first;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",one_only,rustium_auth_first,unique,2
	.globl	rustium_auth_first
	.p2align	4
rustium_auth_first:
.seh_proc rustium_auth_first
	pushq	%rsi
	.seh_pushreg %rsi
	subq	$32, %rsp
	.seh_stackalloc 32
	.seh_endprologue
	movl	$3, %eax
	cmpq	%r9, %rdx
	jne	.LBB2_2
	movq	80(%rsp), %rsi
	testq	%rdx, %rdx
	movl	$1, %eax
	cmoveq	%rax, %r8
	cmoveq	%rax, %rcx
	movq	%rdx, %rax
	movq	%r8, %rdx
	movq	%rax, %r8
	callq	memcmp
	xorl	%ecx, %ecx
	testl	%eax, %eax
	setne	%cl
	testq	%rsi, %rsi
	leal	(%rcx,%rcx,2), %ecx
	movl	$3, %eax
	cmovel	%ecx, %eax
.LBB2_2:
	.seh_startepilogue
	addq	$32, %rsp
	popq	%rsi
	.seh_endepilogue
	retq
	.seh_endproc

	.def	rustium_auth_later_decision;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",one_only,rustium_auth_later_decision,unique,3
	.globl	rustium_auth_later_decision
	.p2align	4
rustium_auth_later_decision:
.seh_proc rustium_auth_later_decision
	pushq	%rsi
	.seh_pushreg %rsi
	subq	$32, %rsp
	.seh_stackalloc 32
	.seh_endprologue
	movl	$3, %esi
	cmpq	%r9, %rdx
	jne	.LBB3_3
	testq	%rdx, %rdx
	movl	$1, %eax
	cmoveq	%rax, %r8
	cmoveq	%rax, %rcx
	movq	%rdx, %rax
	movq	%r8, %rdx
	movq	%rax, %r8
	callq	memcmp
	testl	%eax, %eax
	jne	.LBB3_3
	xorl	%eax, %eax
	cmpq	$0, 80(%rsp)
	setne	%al
	leal	(%rax,%rax,2), %esi
	incl	%esi
.LBB3_3:
	movl	%esi, %eax
	.seh_startepilogue
	addq	$32, %rsp
	popq	%rsi
	.seh_endepilogue
	retq
	.seh_endproc

	.def	rustium_auth_tokenize;
	.scl	2;
	.type	32;
	.endef
	.globl	__xmm@bfbfbfbfbfbfbfbfbfbfbfbfbfbfbfbf
	.section	.rdata,"dr",discard,__xmm@bfbfbfbfbfbfbfbfbfbfbfbfbfbfbfbf
	.p2align	4, 0x0
__xmm@bfbfbfbfbfbfbfbfbfbfbfbfbfbfbfbf:
	.zero	16,191
	.globl	__xmm@19191919191919191919191919191919
	.section	.rdata,"dr",discard,__xmm@19191919191919191919191919191919
	.p2align	4, 0x0
__xmm@19191919191919191919191919191919:
	.zero	16,25
	.globl	__xmm@20202020202020202020202020202020
	.section	.rdata,"dr",discard,__xmm@20202020202020202020202020202020
	.p2align	4, 0x0
__xmm@20202020202020202020202020202020:
	.zero	16,32
	.globl	__xmm@0000000000000000bfbfbfbfbfbfbfbf
	.section	.rdata,"dr",discard,__xmm@0000000000000000bfbfbfbfbfbfbfbf
	.p2align	4, 0x0
__xmm@0000000000000000bfbfbfbfbfbfbfbf:
	.byte	191
	.byte	191
	.byte	191
	.byte	191
	.byte	191
	.byte	191
	.byte	191
	.byte	191
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.globl	__xmm@00000000000000001a1a1a1a1a1a1a1a
	.section	.rdata,"dr",discard,__xmm@00000000000000001a1a1a1a1a1a1a1a
	.p2align	4, 0x0
__xmm@00000000000000001a1a1a1a1a1a1a1a:
	.byte	26
	.byte	26
	.byte	26
	.byte	26
	.byte	26
	.byte	26
	.byte	26
	.byte	26
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.globl	__xmm@00000000000000002020202020202020
	.section	.rdata,"dr",discard,__xmm@00000000000000002020202020202020
	.p2align	4, 0x0
__xmm@00000000000000002020202020202020:
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.zero	1
	.section	.text,"xr",one_only,rustium_auth_tokenize,unique,4
	.globl	rustium_auth_tokenize
	.p2align	4
rustium_auth_tokenize:
.seh_proc rustium_auth_tokenize
	pushq	%r14
	.seh_pushreg %r14
	pushq	%rsi
	.seh_pushreg %rsi
	pushq	%rdi
	.seh_pushreg %rdi
	pushq	%rbx
	.seh_pushreg %rbx
	subq	$120, %rsp
	.seh_stackalloc 120
	movdqa	%xmm7, 96(%rsp)
	.seh_savexmm %xmm7, 96
	movdqa	%xmm6, 80(%rsp)
	.seh_savexmm %xmm6, 80
	.seh_endprologue
	testq	%r8, %r8
	movl	$1, %r11d
	cmovneq	%rdx, %r11
	je	.LBB4_5
	leaq	(%r11,%r8), %rsi
	xorl	%eax, %eax
	jmp	.LBB4_2
	.p2align	4
.LBB4_4:
	leaq	(%rdx,%rax), %r10
	incq	%r10
	incq	%rax
	cmpq	%rsi, %r10
	je	.LBB4_5
.LBB4_2:
	movzbl	(%rdx,%rax), %r10d
	cmpl	$32, %r10d
	je	.LBB4_4
	cmpl	$9, %r10d
	je	.LBB4_4
	leaq	(%rdx,%rax), %rdi
	xorl	%r10d, %r10d
	.p2align	4
.LBB4_8:
	leaq	(%rdx,%r10), %rbx
	movzbl	(%rax,%rbx), %ebx
	cmpl	$9, %ebx
	je	.LBB4_12
	cmpl	$32, %ebx
	je	.LBB4_12
	leaq	(%r10,%rdi), %rbx
	incq	%rbx
	incq	%r10
	cmpq	%rsi, %rbx
	jne	.LBB4_8
	movq	%r8, %rdi
	movq	%r8, %r10
	jmp	.LBB4_24
.LBB4_5:
	pxor	%xmm0, %xmm0
	movdqu	%xmm0, 16(%rcx)
	movdqu	%xmm0, (%rcx)
.LBB4_6:
	movq	%rcx, %rax
.LBB4_45:
	movaps	80(%rsp), %xmm6
	movaps	96(%rsp), %xmm7
	.seh_startepilogue
	addq	$120, %rsp
	popq	%rbx
	popq	%rdi
	popq	%rsi
	popq	%r14
	.seh_endepilogue
	retq
.LBB4_12:
	addq	%rax, %r10
	movq	%r10, %rdi
	cmpq	%r8, %r10
	jae	.LBB4_16
	movq	%r10, %rdi
	jmp	.LBB4_14
	.p2align	4
.LBB4_22:
	incq	%rdi
	cmpq	%rdi, %r8
	je	.LBB4_23
.LBB4_14:
	movzbl	(%r11,%rdi), %esi
	cmpl	$32, %esi
	je	.LBB4_22
	cmpl	$9, %esi
	je	.LBB4_22
.LBB4_16:
	movq	%r8, %r11
	cmpq	%r8, %rdi
	jb	.LBB4_17
	jmp	.LBB4_25
	.p2align	4
.LBB4_20:
	movq	%rsi, %r11
	cmpq	%rsi, %rdi
	jae	.LBB4_21
.LBB4_17:
	leaq	-1(%r11), %rsi
	cmpq	%r8, %rsi
	jae	.LBB4_46
	movzbl	-1(%rdx,%r11), %ebx
	cmpl	$9, %ebx
	je	.LBB4_20
	cmpl	$32, %ebx
	je	.LBB4_20
	jmp	.LBB4_25
.LBB4_23:
	movq	%r8, %rdi
.LBB4_24:
	movq	%r8, %r11
	jmp	.LBB4_25
.LBB4_21:
	movq	%rdi, %r11
.LBB4_25:
	cmpq	%rax, %r10
	setb	%bl
	cmpq	%r8, %r10
	seta	%sil
	orb	%bl, %sil
	jne	.LBB4_48
	movq	%r10, %rsi
	subq	%rax, %rsi
	jne	.LBB4_28
	xorl	%esi, %esi
	movl	$1, %eax
	jmp	.LBB4_43
.LBB4_28:
	movq	%r11, 72(%rsp)
	movq	%r10, 56(%rsp)
	movq	%rax, 48(%rsp)
	movq	%rdx, 40(%rsp)
	movq	%r9, %rbx
	movq	%rcx, 64(%rsp)
	callq	_RNvCs6d6XRwTwhVh_7___rustc35___rust_no_alloc_shim_is_unstable_v2
	movl	$1, %edx
	movq	%rsi, %rcx
	callq	_RNvCs6d6XRwTwhVh_7___rustc12___rust_alloc
	testq	%rax, %rax
	je	.LBB4_47
	cmpq	$8, %rsi
	movq	%rbx, %rcx
	movq	40(%rsp), %rdx
	movq	48(%rsp), %r8
	movq	56(%rsp), %r9
	jae	.LBB4_31
	xorl	%r10d, %r10d
	jmp	.LBB4_40
.LBB4_31:
	cmpq	$32, %rsi
	jae	.LBB4_33
	xorl	%r10d, %r10d
	jmp	.LBB4_37
.LBB4_33:
	movq	%rsi, %r10
	andq	$-32, %r10
	movq	%rsi, %r11
	andq	$-32, %r11
	xorl	%ebx, %ebx
	movdqa	__xmm@bfbfbfbfbfbfbfbfbfbfbfbfbfbfbfbf(%rip), %xmm0
	movdqa	__xmm@19191919191919191919191919191919(%rip), %xmm1
	movdqa	__xmm@20202020202020202020202020202020(%rip), %xmm2
	.p2align	4
.LBB4_34:
	leaq	(%rdx,%rbx), %r14
	movdqu	(%r8,%r14), %xmm3
	movdqu	16(%r8,%r14), %xmm4
	movdqa	%xmm3, %xmm5
	paddb	%xmm0, %xmm5
	movdqa	%xmm4, %xmm6
	paddb	%xmm0, %xmm6
	movdqa	%xmm5, %xmm7
	pminub	%xmm1, %xmm7
	pcmpeqb	%xmm5, %xmm7
	pand	%xmm2, %xmm7
	movdqa	%xmm6, %xmm5
	pminub	%xmm1, %xmm5
	pcmpeqb	%xmm6, %xmm5
	pand	%xmm2, %xmm5
	por	%xmm3, %xmm7
	por	%xmm4, %xmm5
	movdqu	%xmm7, (%rax,%rbx)
	movdqu	%xmm5, 16(%rax,%rbx)
	addq	$32, %rbx
	cmpq	%rbx, %r11
	jne	.LBB4_34
	cmpq	%r10, %rsi
	je	.LBB4_42
	testb	$24, %sil
	je	.LBB4_40
.LBB4_37:
	movq	%r10, %r11
	movq	%rsi, %r10
	andq	$-8, %r10
	movq	%rsi, %rbx
	andq	$-8, %rbx
	movdqa	__xmm@0000000000000000bfbfbfbfbfbfbfbf(%rip), %xmm0
	movdqa	__xmm@00000000000000001a1a1a1a1a1a1a1a(%rip), %xmm1
	movdqa	__xmm@00000000000000002020202020202020(%rip), %xmm2
	.p2align	4
.LBB4_38:
	leaq	(%rdx,%r11), %r14
	movq	(%r8,%r14), %xmm3
	movdqa	%xmm3, %xmm4
	paddb	%xmm0, %xmm4
	movdqa	%xmm4, %xmm5
	pmaxub	%xmm1, %xmm5
	pcmpeqb	%xmm4, %xmm5
	pandn	%xmm2, %xmm5
	por	%xmm3, %xmm5
	movq	%xmm5, (%rax,%r11)
	addq	$8, %r11
	cmpq	%r11, %rbx
	jne	.LBB4_38
	cmpq	%r10, %rsi
	je	.LBB4_42
.LBB4_40:
	subq	%r8, %r9
	.p2align	4
.LBB4_41:
	leaq	(%rdx,%r10), %r11
	movzbl	(%r8,%r11), %r11d
	leal	-65(%r11), %ebx
	cmpb	$26, %bl
	setb	%bl
	shlb	$5, %bl
	orb	%r11b, %bl
	movb	%bl, (%rax,%r10)
	incq	%r10
	cmpq	%r10, %r9
	jne	.LBB4_41
.LBB4_42:
	movq	%rax, %rdx
	movq	%rsi, %r8
	movq	%rax, %rbx
	callq	memcpy
	movq	%rbx, %rax
	movq	64(%rsp), %rcx
	movq	72(%rsp), %r11
.LBB4_43:
	xorl	%edx, %edx
	subq	%rdi, %r11
	cmovaeq	%r11, %rdx
	movq	%rsi, (%rcx)
	movq	%rdi, 8(%rcx)
	movq	%rdx, 16(%rcx)
	movq	$1, 24(%rcx)
	testq	%rsi, %rsi
	je	.LBB4_6
	movl	$1, %r8d
	movq	%rcx, %rdi
	movq	%rax, %rcx
	movq	%rsi, %rdx
	callq	_RNvCs6d6XRwTwhVh_7___rustc14___rust_dealloc
	movq	%rdi, %rax
	jmp	.LBB4_45
.LBB4_48:
	leaq	anon.fde56726e783d76ca05f96b9a8b0bccb.4(%rip), %r9
	movq	%rax, %rcx
	movq	%r10, %rdx
	callq	_RNvNtNtCs9l4wcBNbII5_4core5slice5index16slice_index_fail
	ud2
.LBB4_46:
	decq	%r11
	leaq	anon.fde56726e783d76ca05f96b9a8b0bccb.3(%rip), %rax
	movq	%r11, %rcx
	movq	%r8, %rdx
	movq	%rax, %r8
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_bounds_check
	ud2
.LBB4_47:
	movl	$1, %ecx
	movq	%rsi, %rdx
	callq	_RNvNtCs8tsiLsQ9XHg_5alloc7raw_vec12handle_error
	ud2
	.seh_endproc

	.section	.rdata,"dr",one_only,anon.fde56726e783d76ca05f96b9a8b0bccb.0,unique,5
anon.fde56726e783d76ca05f96b9a8b0bccb.0:
	.asciz	"net/http\\auth_challenge_core.rs"

	.section	.rdata,"dr",one_only,anon.fde56726e783d76ca05f96b9a8b0bccb.1,unique,6
	.p2align	3, 0x0
anon.fde56726e783d76ca05f96b9a8b0bccb.1:
	.quad	anon.fde56726e783d76ca05f96b9a8b0bccb.0
	.asciz	"\037\000\000\000\000\000\000\000`\000\000\000\030\000\000"

	.section	.rdata,"dr",one_only,anon.fde56726e783d76ca05f96b9a8b0bccb.2,unique,7
	.p2align	3, 0x0
anon.fde56726e783d76ca05f96b9a8b0bccb.2:
	.quad	anon.fde56726e783d76ca05f96b9a8b0bccb.0
	.asciz	"\037\000\000\000\000\000\000\000^\000\000\000\027\000\000"

	.section	.rdata,"dr",one_only,anon.fde56726e783d76ca05f96b9a8b0bccb.3,unique,8
	.p2align	3, 0x0
anon.fde56726e783d76ca05f96b9a8b0bccb.3:
	.quad	anon.fde56726e783d76ca05f96b9a8b0bccb.0
	.asciz	"\037\000\000\000\000\000\000\000\036\000\000\000*\000\000"

	.section	.rdata,"dr",one_only,anon.fde56726e783d76ca05f96b9a8b0bccb.4,unique,9
	.p2align	3, 0x0
anon.fde56726e783d76ca05f96b9a8b0bccb.4:
	.quad	anon.fde56726e783d76ca05f96b9a8b0bccb.0
	.asciz	"\037\000\000\000\000\000\000\000\"\000\000\000\026\000\000"

	.section	.rdata,"dr",one_only,anon.fde56726e783d76ca05f96b9a8b0bccb.5,unique,10
	.p2align	3, 0x0
anon.fde56726e783d76ca05f96b9a8b0bccb.5:
	.quad	anon.fde56726e783d76ca05f96b9a8b0bccb.0
	.asciz	"\037\000\000\000\000\000\000\000*\000\000\000&\000\000"

