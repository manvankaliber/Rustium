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
	pushq	%rsi
	.seh_pushreg %rsi
	pushq	%rdi
	.seh_pushreg %rdi
	pushq	%rbx
	.seh_pushreg %rbx
	subq	$32, %rsp
	.seh_stackalloc 32
	.seh_endprologue
	testq	%rdx, %rdx
	je	.LBB0_1
	cmpq	$4, %rdx
	setb	%al
	testb	$3, %dl
	setne	%r10b
	orb	%al, %r10b
	jne	.LBB0_42
	cmpb	$61, -1(%rcx,%rdx)
	jne	.LBB0_4
	leaq	-1(%rdx), %rax
	cmpb	$61, -2(%rcx,%rdx)
	leaq	-2(%rdx), %r10
	cmoveq	%r10, %rax
	cmpq	%rdx, %rax
	ja	.LBB0_13
	movq	%rax, %rdx
.LBB0_4:
	xorl	%r10d, %r10d
	jmp	.LBB0_5
	.p2align	4
.LBB0_26:
	addq	%r11, %rcx
	movq	%rax, %r10
	subq	%r11, %rdx
	je	.LBB0_43
.LBB0_5:
	cmpq	$4, %rdx
	movl	$4, %r11d
	cmovbq	%rdx, %r11
	movzbl	(%rcx), %esi
	leal	-65(%rsi), %eax
	cmpb	$26, %al
	jb	.LBB0_17
	leal	-97(%rsi), %eax
	cmpb	$26, %al
	jae	.LBB0_7
	addb	$-71, %sil
	jmp	.LBB0_16
	.p2align	4
.LBB0_7:
	leal	-48(%rsi), %eax
	cmpb	$10, %al
	jae	.LBB0_8
	addb	$4, %sil
.LBB0_16:
	movl	%esi, %eax
.LBB0_17:
	cmpq	$1, %rdx
	je	.LBB0_49
	movzbl	1(%rcx), %edi
	leal	-65(%rdi), %esi
	cmpb	$26, %sil
	jb	.LBB0_24
	leal	-97(%rdi), %esi
	cmpb	$26, %sil
	jae	.LBB0_20
	addb	$-71, %dil
	movl	%edi, %esi
	jmp	.LBB0_24
	.p2align	4
.LBB0_20:
	leal	-48(%rdi), %esi
	cmpb	$10, %sil
	jae	.LBB0_21
	addb	$4, %dil
	movl	%edi, %esi
	jmp	.LBB0_24
.LBB0_8:
	movb	$62, %al
	cmpl	$43, %esi
	je	.LBB0_17
	cmpl	$47, %esi
	jne	.LBB0_42
	movb	$63, %al
	jmp	.LBB0_17
.LBB0_21:
	movb	$62, %sil
	cmpl	$43, %edi
	je	.LBB0_24
	cmpl	$47, %edi
	jne	.LBB0_42
	movb	$63, %sil
	.p2align	4
.LBB0_24:
	cmpq	%r9, %r10
	jae	.LBB0_42
	movl	%esi, %ebx
	shrb	$4, %bl
	shlb	$2, %al
	orb	%bl, %al
	movb	%al, (%r8,%r10)
	leaq	1(%r10), %rax
	cmpq	$2, %rdx
	jbe	.LBB0_26
	movzbl	2(%rcx), %ebx
	leal	-65(%rbx), %edi
	cmpb	$26, %dil
	jb	.LBB0_33
	leal	-97(%rbx), %edi
	cmpb	$26, %dil
	jae	.LBB0_29
	addb	$-71, %bl
	movl	%ebx, %edi
	jmp	.LBB0_33
.LBB0_29:
	leal	-48(%rbx), %edi
	cmpb	$10, %dil
	jae	.LBB0_30
	addb	$4, %bl
	movl	%ebx, %edi
	jmp	.LBB0_33
.LBB0_30:
	movb	$62, %dil
	cmpl	$43, %ebx
	je	.LBB0_33
	cmpl	$47, %ebx
	jne	.LBB0_42
	movb	$63, %dil
	.p2align	4
.LBB0_33:
	cmpq	%r9, %rax
	jae	.LBB0_42
	movl	%edi, %eax
	shrb	$2, %al
	shlb	$4, %sil
	orb	%al, %sil
	movb	%sil, 1(%r8,%r10)
	leaq	2(%r10), %rax
	cmpq	$3, %rdx
	je	.LBB0_26
	movzbl	3(%rcx), %esi
	leal	-65(%rsi), %ebx
	cmpb	$26, %bl
	jb	.LBB0_41
	leal	-97(%rsi), %ebx
	cmpb	$26, %bl
	jae	.LBB0_37
	addb	$-71, %sil
	movl	%esi, %ebx
	jmp	.LBB0_41
.LBB0_37:
	leal	-48(%rsi), %ebx
	cmpb	$10, %bl
	jae	.LBB0_38
	addb	$4, %sil
	movl	%esi, %ebx
	jmp	.LBB0_41
.LBB0_38:
	movb	$62, %bl
	cmpl	$43, %esi
	je	.LBB0_41
	cmpl	$47, %esi
	jne	.LBB0_42
	movb	$63, %bl
	.p2align	4
.LBB0_41:
	cmpq	%r9, %rax
	jae	.LBB0_42
	shlb	$6, %dil
	orb	%dil, %bl
	movb	%bl, 2(%r8,%r10)
	addq	$3, %r10
	movq	%r10, %rax
	jmp	.LBB0_26
.LBB0_42:
	movq	$-1, %rax
	jmp	.LBB0_43
.LBB0_1:
	xorl	%eax, %eax
.LBB0_43:
	.seh_startepilogue
	addq	$32, %rsp
	popq	%rbx
	popq	%rdi
	popq	%rsi
	.seh_endepilogue
	retq
.LBB0_13:
	leaq	anon.99f189d1bbe2daa5a6b0aae45ea32451.3(%rip), %r9
	xorl	%ecx, %ecx
	movq	%rdx, %r8
	movq	%rax, %rdx
	callq	_RNvNtNtCs9l4wcBNbII5_4core5slice5index16slice_index_fail
	ud2
.LBB0_49:
	leaq	anon.99f189d1bbe2daa5a6b0aae45ea32451.2(%rip), %r8
	movl	$1, %ecx
	movl	$1, %edx
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_bounds_check
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
	leaq	anon.99f189d1bbe2daa5a6b0aae45ea32451.1(%rip), %r8
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
	movl	$3, %esi
	cmpq	%r9, %rdx
	jne	.LBB2_16
	testq	%rdx, %rdx
	movl	$1, %eax
	cmoveq	%rax, %r8
	cmoveq	%rax, %rcx
	movq	%rdx, %rax
	movq	%r8, %rdx
	movq	%rax, %r8
	callq	memcmp
	testl	%eax, %eax
	jne	.LBB2_16
	movq	88(%rsp), %rdx
	movabsq	$9223372036854775804, %rax
	andq	%rdx, %rax
	testb	$3, %dl
	je	.LBB2_14
	movq	%rdx, %rcx
	subq	$1, %rcx
	jb	.LBB2_18
	movq	80(%rsp), %r8
	cmpb	$61, (%r8,%rcx)
	jne	.LBB2_12
	testb	$3, %cl
	je	.LBB2_14
	leaq	-2(%rdx), %r9
	cmpq	$1, %rdx
	je	.LBB2_17
	cmpb	$61, (%r8,%r9)
	jne	.LBB2_15
	testb	$3, %r9b
	je	.LBB2_14
	movq	%rdx, %rcx
	subq	$3, %rcx
	jb	.LBB2_18
	cmpb	$61, (%r8,%rcx)
	cmoveq	%rax, %r9
	movq	%r9, %rcx
	jmp	.LBB2_15
.LBB2_14:
	movq	%rax, %rcx
	jmp	.LBB2_15
.LBB2_12:
	movq	%rdx, %rcx
.LBB2_15:
	xorl	%eax, %eax
	testq	%rcx, %rcx
	setne	%al
	leal	(%rax,%rax,2), %esi
.LBB2_16:
	movl	%esi, %eax
	.seh_startepilogue
	addq	$32, %rsp
	popq	%rsi
	.seh_endepilogue
	retq
.LBB2_17:
	movq	%r9, %rcx
.LBB2_18:
	leaq	anon.99f189d1bbe2daa5a6b0aae45ea32451.1(%rip), %r8
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_bounds_check
	ud2
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
	pushq	%rdi
	.seh_pushreg %rdi
	subq	$40, %rsp
	.seh_stackalloc 40
	.seh_endprologue
	movq	%rcx, %rsi
	movl	$3, %edi
	cmpq	96(%rsp), %r8
	jne	.LBB3_3
	testq	%r8, %r8
	movl	$1, %eax
	cmoveq	%rax, %r9
	cmoveq	%rax, %rdx
	movq	%rdx, %rcx
	movq	%r9, %rdx
	callq	memcmp
	testl	%eax, %eax
	je	.LBB3_4
.LBB3_3:
	xorl	%ecx, %ecx
.LBB3_18:
	movq	%rcx, (%rsi)
	movq	%rdi, 8(%rsi)
	movq	%rsi, %rax
	.seh_startepilogue
	addq	$40, %rsp
	popq	%rdi
	popq	%rsi
	.seh_endepilogue
	retq
.LBB3_4:
	movq	112(%rsp), %rdx
	movabsq	$9223372036854775804, %rax
	andq	%rdx, %rax
	testb	$3, %dl
	je	.LBB3_16
	movq	%rdx, %rcx
	subq	$1, %rcx
	jb	.LBB3_20
	movq	104(%rsp), %r8
	cmpb	$61, (%r8,%rcx)
	jne	.LBB3_14
	testb	$3, %cl
	je	.LBB3_16
	leaq	-2(%rdx), %r9
	cmpq	$1, %rdx
	je	.LBB3_19
	cmpb	$61, (%r8,%r9)
	jne	.LBB3_17
	testb	$3, %r9b
	je	.LBB3_16
	movq	%rdx, %rcx
	subq	$3, %rcx
	jb	.LBB3_20
	cmpb	$61, (%r8,%rcx)
	cmoveq	%rax, %r9
	movq	%r9, %rcx
	jmp	.LBB3_17
.LBB3_16:
	movq	%rax, %rcx
	jmp	.LBB3_17
.LBB3_14:
	movq	%rdx, %rcx
.LBB3_17:
	xorl	%eax, %eax
	testq	%rcx, %rcx
	setne	%al
	leaq	(%rax,%rax,2), %rdi
	incq	%rdi
	jmp	.LBB3_18
.LBB3_19:
	movq	%r9, %rcx
.LBB3_20:
	leaq	anon.99f189d1bbe2daa5a6b0aae45ea32451.1(%rip), %r8
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_bounds_check
	ud2
	.seh_endproc

	.def	rustium_auth_lower_in_place;
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
	.section	.text,"xr",one_only,rustium_auth_lower_in_place,unique,4
	.globl	rustium_auth_lower_in_place
	.p2align	4
rustium_auth_lower_in_place:
.seh_proc rustium_auth_lower_in_place
	subq	$40, %rsp
	.seh_stackalloc 40
	movdqa	%xmm7, 16(%rsp)
	.seh_savexmm %xmm7, 16
	movdqa	%xmm6, (%rsp)
	.seh_savexmm %xmm6, 0
	.seh_endprologue
	testq	%rdx, %rdx
	je	.LBB4_13
	cmpq	$8, %rdx
	jae	.LBB4_4
	xorl	%eax, %eax
	jmp	.LBB4_3
.LBB4_4:
	cmpq	$32, %rdx
	jae	.LBB4_6
	xorl	%eax, %eax
	jmp	.LBB4_10
.LBB4_6:
	movq	%rdx, %rax
	andq	$-32, %rax
	xorl	%r8d, %r8d
	movdqa	__xmm@bfbfbfbfbfbfbfbfbfbfbfbfbfbfbfbf(%rip), %xmm0
	movdqa	__xmm@19191919191919191919191919191919(%rip), %xmm1
	movdqa	__xmm@20202020202020202020202020202020(%rip), %xmm2
	.p2align	4
.LBB4_7:
	movdqu	(%rcx,%r8), %xmm3
	movdqu	16(%rcx,%r8), %xmm4
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
	movdqu	%xmm7, (%rcx,%r8)
	movdqu	%xmm5, 16(%rcx,%r8)
	addq	$32, %r8
	cmpq	%r8, %rax
	jne	.LBB4_7
	cmpq	%rax, %rdx
	je	.LBB4_13
	testb	$24, %dl
	je	.LBB4_3
.LBB4_10:
	movq	%rax, %r8
	movq	%rdx, %rax
	andq	$-8, %rax
	movdqa	__xmm@0000000000000000bfbfbfbfbfbfbfbf(%rip), %xmm0
	movdqa	__xmm@00000000000000001a1a1a1a1a1a1a1a(%rip), %xmm1
	movdqa	__xmm@00000000000000002020202020202020(%rip), %xmm2
	.p2align	4
.LBB4_11:
	movq	(%rcx,%r8), %xmm3
	movdqa	%xmm3, %xmm4
	paddb	%xmm0, %xmm4
	movdqa	%xmm4, %xmm5
	pmaxub	%xmm1, %xmm5
	pcmpeqb	%xmm4, %xmm5
	pandn	%xmm2, %xmm5
	por	%xmm3, %xmm5
	movq	%xmm5, (%rcx,%r8)
	addq	$8, %r8
	cmpq	%r8, %rax
	jne	.LBB4_11
	cmpq	%rax, %rdx
	je	.LBB4_13
.LBB4_3:
	movzbl	(%rcx,%rax), %r8d
	leal	-65(%r8), %r9d
	cmpb	$26, %r9b
	setb	%r9b
	shlb	$5, %r9b
	orb	%r8b, %r9b
	movb	%r9b, (%rcx,%rax)
	incq	%rax
	cmpq	%rax, %rdx
	jne	.LBB4_3
.LBB4_13:
	movaps	(%rsp), %xmm6
	movaps	16(%rsp), %xmm7
	.seh_startepilogue
	addq	$40, %rsp
	.seh_endepilogue
	retq
	.seh_endproc

	.def	rustium_auth_tokenize;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",one_only,rustium_auth_tokenize,unique,5
	.globl	rustium_auth_tokenize
	.p2align	4
rustium_auth_tokenize:
.seh_proc rustium_auth_tokenize
	pushq	%r15
	.seh_pushreg %r15
	pushq	%r14
	.seh_pushreg %r14
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
	movdqa	%xmm7, 48(%rsp)
	.seh_savexmm %xmm7, 48
	movdqa	%xmm6, 32(%rsp)
	.seh_savexmm %xmm6, 32
	.seh_endprologue
	testq	%r8, %r8
	movl	$1, %esi
	cmovneq	%rdx, %rsi
	je	.LBB5_1
	movq	%rdx, %r11
	leaq	(%rsi,%r8), %r10
	xorl	%edx, %edx
	xorl	%eax, %eax
	jmp	.LBB5_3
	.p2align	4
.LBB5_5:
	leaq	(%r11,%rax), %rdi
	incq	%rdi
	incq	%rax
	cmpq	%r10, %rdi
	je	.LBB5_6
.LBB5_3:
	movzbl	(%r11,%rax), %edi
	cmpl	$32, %edi
	je	.LBB5_5
	cmpl	$9, %edi
	je	.LBB5_5
	movq	%rax, %rdx
	negq	%rdx
	leaq	(%r11,%rax), %rdi
	xorl	%ebx, %ebx
	.p2align	4
.LBB5_8:
	leaq	(%r11,%rbx), %r14
	movzbl	(%rax,%r14), %ebp
	cmpl	$9, %ebp
	je	.LBB5_12
	cmpl	$32, %ebp
	je	.LBB5_12
	decq	%rdx
	leaq	(%rbx,%rdi), %r14
	incq	%r14
	incq	%rbx
	cmpq	%r10, %r14
	jne	.LBB5_8
	movq	%r8, %rdx
.LBB5_23:
	movq	%r8, %r10
	movq	%r8, %rdi
	jmp	.LBB5_24
.LBB5_1:
	xorl	%edx, %edx
.LBB5_6:
	xorl	%r8d, %r8d
	xorl	%edi, %edi
	xorl	%eax, %eax
	xorl	%r11d, %r11d
	jmp	.LBB5_42
.LBB5_12:
	negq	%rdx
	movq	%rdx, %rdi
	cmpq	%r8, %rdx
	jae	.LBB5_16
	movq	%rdx, %rdi
	jmp	.LBB5_14
	.p2align	4
.LBB5_22:
	incq	%rdi
	cmpq	%rdi, %r8
	je	.LBB5_23
.LBB5_14:
	movzbl	(%rsi,%rdi), %r10d
	cmpl	$32, %r10d
	je	.LBB5_22
	cmpl	$9, %r10d
	je	.LBB5_22
.LBB5_16:
	movq	%r8, %r10
	cmpq	%r8, %rdi
	jb	.LBB5_17
	jmp	.LBB5_24
	.p2align	4
.LBB5_20:
	movq	%rbx, %r10
	cmpq	%rbx, %rdi
	jae	.LBB5_21
.LBB5_17:
	leaq	-1(%r10), %rbx
	cmpq	%r8, %rbx
	jae	.LBB5_43
	movzbl	-1(%r11,%r10), %ebp
	cmpl	$9, %ebp
	je	.LBB5_20
	cmpl	$32, %ebp
	je	.LBB5_20
	jmp	.LBB5_24
.LBB5_21:
	movq	%rdi, %r10
.LBB5_24:
	movq	%rdx, %r11
	subq	%rax, %r11
	seta	%bl
	cmpq	160(%rsp), %r11
	seta	%bpl
	testb	%bpl, %bl
	jne	.LBB5_41
	testq	%r11, %r11
	movl	$1, %r14d
	cmoveq	%r14, %r9
	xorl	%r15d, %r15d
	testb	%bl, %bl
	cmovneq	%r11, %r15
	cmoveq	%r14, %r9
	cmpq	%rax, %rdx
	setb	%bpl
	cmpq	%r8, %rdx
	seta	%r14b
	orb	%bpl, %r14b
	jne	.LBB5_44
	movq	%r15, %rbx
	testq	%rbx, %rbx
	je	.LBB5_27
	movq	%rbx, %rdx
	cmpq	$8, %rbx
	jae	.LBB5_30
	xorl	%r8d, %r8d
	jmp	.LBB5_39
.LBB5_27:
	xorl	%r11d, %r11d
	jmp	.LBB5_41
.LBB5_30:
	cmpq	$32, %rdx
	jae	.LBB5_32
	xorl	%r8d, %r8d
	jmp	.LBB5_36
.LBB5_32:
	movq	%rdx, %r8
	andq	$-32, %r8
	xorl	%ebx, %ebx
	movdqa	__xmm@bfbfbfbfbfbfbfbfbfbfbfbfbfbfbfbf(%rip), %xmm0
	movdqa	__xmm@19191919191919191919191919191919(%rip), %xmm1
	movdqa	__xmm@20202020202020202020202020202020(%rip), %xmm2
	.p2align	4
.LBB5_33:
	leaq	(%rsi,%rbx), %r14
	movdqu	(%rax,%r14), %xmm3
	movdqu	16(%rax,%r14), %xmm4
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
	movdqu	%xmm7, (%r9,%rbx)
	movdqu	%xmm5, 16(%r9,%rbx)
	addq	$32, %rbx
	cmpq	%rbx, %r8
	jne	.LBB5_33
	cmpq	%r8, %rdx
	je	.LBB5_41
	testb	$24, %dl
	je	.LBB5_39
.LBB5_36:
	movq	%r8, %rbx
	movq	%rdx, %r8
	andq	$-8, %r8
	movdqa	__xmm@0000000000000000bfbfbfbfbfbfbfbf(%rip), %xmm0
	movdqa	__xmm@00000000000000001a1a1a1a1a1a1a1a(%rip), %xmm1
	movdqa	__xmm@00000000000000002020202020202020(%rip), %xmm2
	.p2align	4
.LBB5_37:
	leaq	(%rsi,%rbx), %r14
	movq	(%rax,%r14), %xmm3
	movdqa	%xmm3, %xmm4
	paddb	%xmm0, %xmm4
	movdqa	%xmm4, %xmm5
	pmaxub	%xmm1, %xmm5
	pcmpeqb	%xmm4, %xmm5
	pandn	%xmm2, %xmm5
	por	%xmm3, %xmm5
	movq	%xmm5, (%r9,%rbx)
	addq	$8, %rbx
	cmpq	%rbx, %r8
	jne	.LBB5_37
	cmpq	%r8, %rdx
	je	.LBB5_41
.LBB5_39:
	addq	%r8, %r9
	subq	%r8, %rdx
	addq	%r8, %rsi
	xorl	%r8d, %r8d
	.p2align	4
.LBB5_40:
	leaq	(%rsi,%r8), %rbx
	movzbl	(%rax,%rbx), %ebx
	leal	-65(%rbx), %ebp
	cmpb	$26, %bpl
	setb	%bpl
	shlb	$5, %bpl
	orb	%bl, %bpl
	movb	%bpl, (%r9,%r8)
	incq	%r8
	cmpq	%r8, %rdx
	jne	.LBB5_40
.LBB5_41:
	xorl	%edx, %edx
	subq	%rdi, %r10
	cmovaeq	%r10, %rdx
	movl	$1, %r8d
.LBB5_42:
	movq	%rax, (%rcx)
	movq	%r11, 8(%rcx)
	movq	%rdi, 16(%rcx)
	movq	%rdx, 24(%rcx)
	movq	%r8, 32(%rcx)
	movq	%rcx, %rax
	movaps	32(%rsp), %xmm6
	movaps	48(%rsp), %xmm7
	.seh_startepilogue
	addq	$72, %rsp
	popq	%rbx
	popq	%rbp
	popq	%rdi
	popq	%rsi
	popq	%r14
	popq	%r15
	.seh_endepilogue
	retq
.LBB5_44:
	leaq	anon.99f189d1bbe2daa5a6b0aae45ea32451.6(%rip), %r9
	movq	%rax, %rcx
	callq	_RNvNtNtCs9l4wcBNbII5_4core5slice5index16slice_index_fail
	ud2
.LBB5_43:
	decq	%r10
	leaq	anon.99f189d1bbe2daa5a6b0aae45ea32451.4(%rip), %rax
	movq	%r10, %rcx
	movq	%r8, %rdx
	movq	%rax, %r8
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_bounds_check
	ud2
	.seh_endproc

	.section	.rdata,"dr",one_only,anon.99f189d1bbe2daa5a6b0aae45ea32451.0,unique,6
anon.99f189d1bbe2daa5a6b0aae45ea32451.0:
	.asciz	"net/http\\auth_challenge_core.rs"

	.section	.rdata,"dr",one_only,anon.99f189d1bbe2daa5a6b0aae45ea32451.1,unique,7
	.p2align	3, 0x0
anon.99f189d1bbe2daa5a6b0aae45ea32451.1:
	.quad	anon.99f189d1bbe2daa5a6b0aae45ea32451.0
	.asciz	"\037\000\000\000\000\000\000\000)\000\000\000&\000\000"

	.section	.rdata,"dr",one_only,anon.99f189d1bbe2daa5a6b0aae45ea32451.2,unique,8
	.p2align	3, 0x0
anon.99f189d1bbe2daa5a6b0aae45ea32451.2:
	.quad	anon.99f189d1bbe2daa5a6b0aae45ea32451.0
	.asciz	"\037\000\000\000\000\000\000\000a\000\000\000\030\000\000"

	.section	.rdata,"dr",one_only,anon.99f189d1bbe2daa5a6b0aae45ea32451.3,unique,9
	.p2align	3, 0x0
anon.99f189d1bbe2daa5a6b0aae45ea32451.3:
	.quad	anon.99f189d1bbe2daa5a6b0aae45ea32451.0
	.asciz	"\037\000\000\000\000\000\000\000_\000\000\000\027\000\000"

	.section	.rdata,"dr",one_only,anon.99f189d1bbe2daa5a6b0aae45ea32451.4,unique,10
	.p2align	3, 0x0
anon.99f189d1bbe2daa5a6b0aae45ea32451.4:
	.quad	anon.99f189d1bbe2daa5a6b0aae45ea32451.0
	.asciz	"\037\000\000\000\000\000\000\000\031\000\000\000*\000\000"

	.section	.rdata,"dr",one_only,anon.99f189d1bbe2daa5a6b0aae45ea32451.5,unique,11
anon.99f189d1bbe2daa5a6b0aae45ea32451.5:
	.asciz	"net/http/auth_challenge_ffi.rs"

	.section	.rdata,"dr",one_only,anon.99f189d1bbe2daa5a6b0aae45ea32451.6,unique,12
	.p2align	3, 0x0
anon.99f189d1bbe2daa5a6b0aae45ea32451.6:
	.quad	anon.99f189d1bbe2daa5a6b0aae45ea32451.5
	.asciz	"\036\000\000\000\000\000\000\0006\000\000\0003\000\000"

