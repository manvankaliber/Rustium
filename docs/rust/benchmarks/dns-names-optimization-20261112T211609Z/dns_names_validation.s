	.def	@feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
@feat.00 = 2048
	.att_syntax
	.file	"dns_names_validation.48ac01837e92f33f-cgu.0"
	.def	_RNvNtCs6ePvCSTRAf3_20dns_names_validation14dns_names_core5valid;
	.scl	3;
	.type	32;
	.endef
	.section	.text,"xr",one_only,_RNvNtCs6ePvCSTRAf3_20dns_names_validation14dns_names_core5valid,unique,0
	.p2align	4
_RNvNtCs6ePvCSTRAf3_20dns_names_validation14dns_names_core5valid:
.seh_proc _RNvNtCs6ePvCSTRAf3_20dns_names_validation14dns_names_core5valid
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
	je	.LBB0_21
	movq	%rdx, %r8
	movq	%rdx, %r9
	negq	%r9
	xorl	%r10d, %r10d
	xorl	%eax, %eax
	.p2align	4
.LBB0_2:
	movq	%r8, %rsi
	subq	%rax, %rsi
	leaq	(%r9,%rax), %rdx
	movq	%rcx, %r11
	xorl	%edi, %edi
	.p2align	4
.LBB0_3:
	cmpb	$46, (%r11,%rax)
	je	.LBB0_6
	decq	%rdi
	incq	%r11
	cmpq	%rdi, %rdx
	jne	.LBB0_3
	movq	%r8, %rdx
	movq	%rsi, %r11
	jmp	.LBB0_8
	.p2align	4
.LBB0_6:
	movq	%rdi, %r11
	negq	%r11
	cmpq	%rsi, %r11
	jae	.LBB0_24
	movq	%rax, %rdx
	subq	%rdi, %rdx
.LBB0_8:
	cmpq	%rax, %rdx
	sete	%bl
	cmpq	$64, %r11
	setae	%sil
	orb	%bl, %sil
	jne	.LBB0_21
	movq	%r11, %rsi
	addq	%r10, %rsi
	jb	.LBB0_25
	cmpq	$-3, %rsi
	ja	.LBB0_25
	cmpq	$253, %rsi
	ja	.LBB0_21
	cmpq	%r8, %rdx
	ja	.LBB0_23
	addq	%r10, %r11
	incq	%r11
	cmpq	%r10, %r11
	jb	.LBB0_26
	incq	%rdx
	movq	%r11, %r10
	movq	%rdx, %rax
	cmpq	%r8, %rdx
	jb	.LBB0_2
	testq	%r11, %r11
	je	.LBB0_21
	cmpq	$-1, %r11
	je	.LBB0_27
	movb	$1, %al
	jmp	.LBB0_22
.LBB0_21:
	xorl	%eax, %eax
.LBB0_22:
	.seh_startepilogue
	addq	$32, %rsp
	popq	%rbx
	popq	%rdi
	popq	%rsi
	.seh_endepilogue
	retq
.LBB0_23:
	leaq	alloc_046d1a7a4881635cfad5b0c2ff7a23f6(%rip), %r9
	movq	%rax, %rcx
	callq	_RNvNtNtCs9l4wcBNbII5_4core5slice5index16slice_index_fail
	ud2
.LBB0_24:
	leaq	alloc_64e308ef4babfeb8b6220184de794a17(%rip), %rcx
	leaq	alloc_1d1b1e35bb489427b2bdc9752de92226(%rip), %r9
	movl	$443, %edx
	xorl	%r8d, %r8d
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_nounwind_fmt
	ud2
.LBB0_25:
	leaq	alloc_936ea8349fd2f15fa257fdf99fc48b3a(%rip), %rcx
	callq	_RNvNtNtCs9l4wcBNbII5_4core9panicking11panic_const24panic_const_add_overflow
	ud2
.LBB0_26:
	leaq	alloc_45ee9b8e8fc13b44318480984e962740(%rip), %rcx
	callq	_RNvNtNtCs9l4wcBNbII5_4core9panicking11panic_const24panic_const_add_overflow
	ud2
.LBB0_27:
	leaq	alloc_95999eda6b30ca646b7466c393e2d425(%rip), %rcx
	callq	_RNvNtNtCs9l4wcBNbII5_4core9panicking11panic_const24panic_const_add_overflow
	ud2
	.seh_endproc

	.def	net$dns_names_internal$cxxbridge1$205$canonical_name;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",one_only,net$dns_names_internal$cxxbridge1$205$canonical_name,unique,1
	.globl	net$dns_names_internal$cxxbridge1$205$canonical_name
	.p2align	4
net$dns_names_internal$cxxbridge1$205$canonical_name:
.seh_proc net$dns_names_internal$cxxbridge1$205$canonical_name
	pushq	%rsi
	.seh_pushreg %rsi
	pushq	%rdi
	.seh_pushreg %rdi
	pushq	%rbx
	.seh_pushreg %rbx
	subq	$48, %rsp
	.seh_stackalloc 48
	.seh_endprologue
	movq	8(%rcx), %rdi
	testq	%rdi, %rdi
	js	.LBB1_4
	movq	%rdx, %rsi
	movq	(%rcx), %rbx
	movq	%rbx, 32(%rsp)
	movq	%rdi, 40(%rsp)
	leaq	32(%rsp), %rcx
	callq	net$dns_names_internal$cxxbridge1$205$host_canonicalize
	testb	%al, %al
	je	.LBB1_3
	movq	%rbx, 32(%rsp)
	movq	%rdi, 40(%rsp)
	leaq	32(%rsp), %rcx
	movq	%rsi, %rdx
	callq	net$dns_names_internal$cxxbridge1$205$canonical_use_original
	nop
.LBB1_3:
	.seh_startepilogue
	addq	$48, %rsp
	popq	%rbx
	popq	%rdi
	popq	%rsi
	.seh_endepilogue
	retq
.LBB1_4:
	leaq	alloc_a28e8c8fd5088943a8b5d44af697ff83(%rip), %rcx
	leaq	alloc_7d52e238cba565908982a5cea838db34(%rip), %r9
	movl	$559, %edx
	xorl	%r8d, %r8d
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_nounwind_fmt
	ud2
	.seh_endproc

	.def	net$dns_names_internal$cxxbridge1$205$dotted_name;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",one_only,net$dns_names_internal$cxxbridge1$205$dotted_name,unique,2
	.globl	net$dns_names_internal$cxxbridge1$205$dotted_name
	.p2align	4
net$dns_names_internal$cxxbridge1$205$dotted_name:
.seh_proc net$dns_names_internal$cxxbridge1$205$dotted_name
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
	pushq	%rbx
	.seh_pushreg %rbx
	subq	$48, %rsp
	.seh_stackalloc 48
	.seh_endprologue
	movq	8(%rcx), %rsi
	testq	%rsi, %rsi
	js	.LBB2_23
	movq	%r8, %rdi
	movq	(%rcx), %r15
	testb	%dl, %dl
	je	.LBB2_3
	movq	%r15, 32(%rsp)
	movq	%rsi, 40(%rsp)
	leaq	32(%rsp), %rcx
	callq	net$dns_names_internal$cxxbridge1$205$host_compliant
	testb	%al, %al
	je	.LBB2_19
.LBB2_3:
	testq	%rsi, %rsi
	je	.LBB2_19
	movq	%rsi, %r13
	negq	%r13
	xorl	%ebx, %ebx
	xorl	%r12d, %r12d
	xorl	%ecx, %ecx
	.p2align	4
.LBB2_5:
	leaq	(%r15,%rcx), %rdx
	movq	%rsi, %rax
	subq	%rcx, %rax
	leaq	(%rcx,%r13), %r8
	movq	%r15, %r10
	xorl	%r9d, %r9d
	.p2align	4
.LBB2_6:
	cmpb	$46, (%r10,%rcx)
	je	.LBB2_9
	decq	%r9
	incq	%r10
	cmpq	%r9, %r8
	jne	.LBB2_6
	movq	%rsi, %r14
	movq	%rax, %r8
	jmp	.LBB2_11
	.p2align	4
.LBB2_9:
	movq	%r9, %r8
	negq	%r8
	cmpq	%rax, %r8
	jae	.LBB2_24
	movq	%rcx, %r14
	subq	%r9, %r14
.LBB2_11:
	cmpq	%rcx, %r14
	sete	%al
	cmpq	$64, %r8
	setae	%r9b
	orb	%al, %r9b
	jne	.LBB2_20
	leaq	(%r8,%r12), %rax
	cmpq	$-3, %rax
	ja	.LBB2_25
	cmpq	$253, %rax
	ja	.LBB2_20
	cmpq	%rsi, %r14
	ja	.LBB2_21
	movb	%r8b, (%rdi,%r12)
	leaq	(%rdi,%r12), %rcx
	incq	%rcx
	movq	%rdx, %rax
	subq	%rcx, %rax
	movq	%rcx, %r9
	subq	%rdx, %r9
	cmovbq	%rax, %r9
	cmpq	%r8, %r9
	jb	.LBB2_22
	incq	%r12
	addq	%r8, %r12
	callq	memcpy
	incq	%r14
	movq	%r14, %rcx
	cmpq	%rsi, %r14
	jb	.LBB2_5
	movb	$0, (%rdi,%r12)
	addq	$2, %r12
	movq	%r12, %rbx
	jmp	.LBB2_20
.LBB2_19:
	xorl	%ebx, %ebx
.LBB2_20:
	movq	%rbx, %rax
	.seh_startepilogue
	addq	$48, %rsp
	popq	%rbx
	popq	%rdi
	popq	%rsi
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	.seh_endepilogue
	retq
.LBB2_21:
	leaq	alloc_046d1a7a4881635cfad5b0c2ff7a23f6(%rip), %r9
	movq	%r14, %rdx
	movq	%rsi, %r8
	callq	_RNvNtNtCs9l4wcBNbII5_4core5slice5index16slice_index_fail
	ud2
.LBB2_22:
	leaq	alloc_bd3468a7b96187f70c1ce98a3e7a63bf(%rip), %rcx
	leaq	alloc_8b2b2beeccfb5776d8a78ad79e89f50b(%rip), %r9
	movl	$567, %edx
	xorl	%r8d, %r8d
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_nounwind_fmt
	ud2
.LBB2_23:
	leaq	alloc_a28e8c8fd5088943a8b5d44af697ff83(%rip), %rcx
	leaq	alloc_7d52e238cba565908982a5cea838db34(%rip), %r9
	movl	$559, %edx
	xorl	%r8d, %r8d
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_nounwind_fmt
	ud2
.LBB2_24:
	leaq	alloc_64e308ef4babfeb8b6220184de794a17(%rip), %rcx
	leaq	alloc_1d1b1e35bb489427b2bdc9752de92226(%rip), %r9
	movl	$443, %edx
	xorl	%r8d, %r8d
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_nounwind_fmt
	ud2
.LBB2_25:
	leaq	alloc_936ea8349fd2f15fa257fdf99fc48b3a(%rip), %rcx
	callq	_RNvNtNtCs9l4wcBNbII5_4core9panicking11panic_const24panic_const_add_overflow
	ud2
	.seh_endproc

	.def	net$dns_names_internal$cxxbridge1$205$network_name;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",one_only,net$dns_names_internal$cxxbridge1$205$network_name,unique,3
	.globl	net$dns_names_internal$cxxbridge1$205$network_name
	.p2align	4
net$dns_names_internal$cxxbridge1$205$network_name:
.seh_proc net$dns_names_internal$cxxbridge1$205$network_name
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
	subq	$56, %rsp
	.seh_stackalloc 56
	.seh_endprologue
	movq	8(%rcx), %r12
	testq	%r12, %r12
	js	.LBB3_27
	movl	%edx, %ebx
	je	.LBB3_14
	movq	%r8, %r14
	movq	(%rcx), %r13
	leaq	40(%rsp), %r15
	xorl	%edi, %edi
	xorl	%esi, %esi
	xorl	%ebp, %ebp
	.p2align	4
.LBB3_3:
	movzbl	(%r13), %r8d
	cmpq	$191, %r8
	ja	.LBB3_16
	leaq	-1(%r12), %rax
	cmpq	%r8, %rax
	jb	.LBB3_16
	leaq	1(%r8), %rax
	addq	%rax, %rsi
	jb	.LBB3_28
	testb	%r8b, %r8b
	je	.LBB3_18
	addq	%rax, %rbp
	cmpb	$64, %r8b
	setae	%cl
	cmpq	$256, %rbp
	setae	%dl
	orb	%cl, %dl
	jne	.LBB3_16
	testq	%rdi, %rdi
	je	.LBB3_11
	cmpq	$255, %rdi
	jae	.LBB3_29
	movb	$46, (%r14,%rdi)
	incq	%rdi
	movq	%rdi, %rcx
	leaq	(%rcx,%r8), %rdi
	cmpq	$256, %rdi
	jb	.LBB3_12
	jmp	.LBB3_25
	.p2align	4
.LBB3_11:
	xorl	%ecx, %ecx
	leaq	(%rcx,%r8), %rdi
	cmpq	$256, %rdi
	jae	.LBB3_25
.LBB3_12:
	addq	%r14, %rcx
	leaq	1(%r13), %rdx
	movq	%rdx, %r9
	subq	%rcx, %r9
	movq	%rcx, %r10
	subq	%rdx, %r10
	cmovbq	%r9, %r10
	cmpq	%r8, %r10
	jb	.LBB3_26
	addq	%r8, %r13
	incq	%r13
	subq	%rax, %r12
	callq	memcpy
	testq	%r12, %r12
	jne	.LBB3_3
	jmp	.LBB3_15
.LBB3_16:
	xorl	%edi, %edi
	movq	%rdi, (%r15)
	cmpl	$1, 40(%rsp)
	je	.LBB3_20
.LBB3_23:
	xorl	%eax, %eax
	jmp	.LBB3_24
.LBB3_14:
	xorl	%esi, %esi
	xorl	%edi, %edi
.LBB3_15:
	xorb	$1, %bl
	movzbl	%bl, %eax
	jmp	.LBB3_19
.LBB3_18:
	movl	$1, %eax
.LBB3_19:
	movq	%rax, 40(%rsp)
	leaq	48(%rsp), %r15
	movq	%rdi, (%r15)
	cmpl	$1, 40(%rsp)
	jne	.LBB3_23
.LBB3_20:
	movq	48(%rsp), %rax
	incq	%rax
	je	.LBB3_21
.LBB3_24:
	shlq	$32, %rsi
	orq	%rax, %rsi
	movq	%rsi, %rax
	.seh_startepilogue
	addq	$56, %rsp
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
.LBB3_25:
	leaq	alloc_7420eb3f3eebbe4d2e8275b2ddfd7058(%rip), %r9
	movl	$255, %r8d
	movq	%rdi, %rdx
	callq	_RNvNtNtCs9l4wcBNbII5_4core5slice5index16slice_index_fail
	ud2
.LBB3_26:
	leaq	alloc_bd3468a7b96187f70c1ce98a3e7a63bf(%rip), %rcx
	leaq	alloc_8b2b2beeccfb5776d8a78ad79e89f50b(%rip), %r9
	movl	$567, %edx
	xorl	%r8d, %r8d
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_nounwind_fmt
	ud2
.LBB3_27:
	leaq	alloc_a28e8c8fd5088943a8b5d44af697ff83(%rip), %rcx
	leaq	alloc_7d52e238cba565908982a5cea838db34(%rip), %r9
	movl	$559, %edx
	xorl	%r8d, %r8d
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_nounwind_fmt
	ud2
.LBB3_28:
	leaq	alloc_e8bce7c582003742c129b5a90af24fe0(%rip), %rcx
	callq	_RNvNtNtCs9l4wcBNbII5_4core9panicking11panic_const24panic_const_add_overflow
	ud2
.LBB3_29:
	leaq	alloc_c0960c38849cb512b20eefc3cdf9c036(%rip), %r8
	movl	$255, %ecx
	movl	$255, %edx
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_bounds_check
	ud2
.LBB3_21:
	leaq	alloc_1d9f78db3a4a8c034b111a5282e4d74b(%rip), %rcx
	callq	_RNvNtNtCs9l4wcBNbII5_4core9panicking11panic_const24panic_const_add_overflow
	ud2
	.seh_endproc

	.def	net$dns_names_internal$cxxbridge1$205$network_name_reader;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",one_only,net$dns_names_internal$cxxbridge1$205$network_name_reader,unique,4
	.globl	net$dns_names_internal$cxxbridge1$205$network_name_reader
	.p2align	4
net$dns_names_internal$cxxbridge1$205$network_name_reader:
.seh_proc net$dns_names_internal$cxxbridge1$205$network_name_reader
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
	subq	$568, %rsp
	.seh_stackalloc 568
	.seh_endprologue
	movb	%dl, 39(%rsp)
	movq	%r8, %r14
	xorps	%xmm0, %xmm0
	movups	%xmm0, 296(%rsp)
	movups	%xmm0, 280(%rsp)
	movups	%xmm0, 264(%rsp)
	movups	%xmm0, 248(%rsp)
	movups	%xmm0, 232(%rsp)
	movups	%xmm0, 216(%rsp)
	movups	%xmm0, 200(%rsp)
	movups	%xmm0, 184(%rsp)
	movups	%xmm0, 168(%rsp)
	movups	%xmm0, 152(%rsp)
	movups	%xmm0, 136(%rsp)
	movups	%xmm0, 120(%rsp)
	movups	%xmm0, 104(%rsp)
	movups	%xmm0, 88(%rsp)
	movups	%xmm0, 72(%rsp)
	movups	%xmm0, 56(%rsp)
	movq	%rcx, 48(%rsp)
	leaq	312(%rsp), %rdx
	callq	net$dns_names_internal$cxxbridge1$205$reader_remaining
	movq	320(%rsp), %rax
	testq	%rax, %rax
	js	.LBB4_8
	leaq	57(%rsp), %r13
	leaq	312(%rsp), %rdi
	xorl	%r15d, %r15d
	xorl	%esi, %esi
	xorl	%r12d, %r12d
	.p2align	4
.LBB4_2:
	testq	%rax, %rax
	je	.LBB4_3
	movq	312(%rsp), %rcx
	movzbl	(%rcx), %ebp
	cmpq	$191, %rbp
	ja	.LBB4_11
	decq	%rax
	cmpq	%rbp, %rax
	jb	.LBB4_11
	leaq	1(%rbp), %rbx
	movq	48(%rsp), %rdx
	movq	%rdi, %rcx
	movq	%rbx, %r8
	callq	net$dns_names_internal$cxxbridge1$205$reader_take
	movl	$256, %r8d
	leaq	56(%rsp), %rcx
	movq	%rdi, %rdx
	callq	memcpy
	addq	%rbx, %rsi
	jb	.LBB4_16
	testb	%bpl, %bpl
	je	.LBB4_15
	addq	%rbx, %r12
	cmpb	$64, %bpl
	setae	%al
	cmpq	$256, %r12
	setae	%cl
	orb	%al, %cl
	jne	.LBB4_11
	testq	%r15, %r15
	je	.LBB4_19
	cmpq	$255, %r15
	jae	.LBB4_24
	movb	$46, (%r14,%r15)
	incq	%r15
	movq	%r15, %rcx
	leaq	(%rcx,%rbp), %r15
	cmpq	$256, %r15
	jb	.LBB4_25
	jmp	.LBB4_21
	.p2align	4
.LBB4_19:
	xorl	%ecx, %ecx
	leaq	(%rcx,%rbp), %r15
	cmpq	$256, %r15
	jae	.LBB4_21
.LBB4_25:
	addq	%r14, %rcx
	movq	%r13, %rax
	subq	%rcx, %rax
	movq	%rcx, %rdx
	subq	%r13, %rdx
	cmovbq	%rax, %rdx
	cmpq	%rbp, %rdx
	jb	.LBB4_26
	movq	%r13, %rdx
	movq	%rbp, %r8
	callq	memcpy
	movq	48(%rsp), %rcx
	movq	%rdi, %rdx
	callq	net$dns_names_internal$cxxbridge1$205$reader_remaining
	movq	320(%rsp), %rax
	testq	%rax, %rax
	jns	.LBB4_2
.LBB4_8:
	leaq	alloc_a28e8c8fd5088943a8b5d44af697ff83(%rip), %rcx
	leaq	alloc_7d52e238cba565908982a5cea838db34(%rip), %r9
	movl	$559, %edx
	xorl	%r8d, %r8d
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_nounwind_fmt
	ud2
.LBB4_11:
	leaq	40(%rsp), %rdi
	xorl	%r15d, %r15d
	movq	%r15, (%rdi)
	cmpl	$1, 40(%rsp)
	jne	.LBB4_6
.LBB4_27:
	movq	312(%rsp), %rax
	incq	%rax
	jne	.LBB4_28
	leaq	alloc_1d9f78db3a4a8c034b111a5282e4d74b(%rip), %rcx
	callq	_RNvNtNtCs9l4wcBNbII5_4core9panicking11panic_const24panic_const_add_overflow
	ud2
.LBB4_3:
	movzbl	39(%rsp), %eax
	xorb	$1, %al
	movzbl	%al, %eax
	jmp	.LBB4_4
.LBB4_15:
	movl	$1, %eax
.LBB4_4:
	movq	%rax, 40(%rsp)
	movq	%r15, (%rdi)
	cmpl	$1, 40(%rsp)
	je	.LBB4_27
.LBB4_6:
	xorl	%eax, %eax
.LBB4_28:
	shlq	$32, %rsi
	orq	%rax, %rsi
	movq	%rsi, %rax
	.seh_startepilogue
	addq	$568, %rsp
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
.LBB4_21:
	leaq	alloc_7420eb3f3eebbe4d2e8275b2ddfd7058(%rip), %r9
	movl	$255, %r8d
	movq	%r15, %rdx
	callq	_RNvNtNtCs9l4wcBNbII5_4core5slice5index16slice_index_fail
	ud2
.LBB4_26:
	leaq	alloc_bd3468a7b96187f70c1ce98a3e7a63bf(%rip), %rcx
	leaq	alloc_8b2b2beeccfb5776d8a78ad79e89f50b(%rip), %r9
	movl	$567, %edx
	xorl	%r8d, %r8d
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_nounwind_fmt
	ud2
.LBB4_16:
	leaq	alloc_e8bce7c582003742c129b5a90af24fe0(%rip), %rcx
	callq	_RNvNtNtCs9l4wcBNbII5_4core9panicking11panic_const24panic_const_add_overflow
	ud2
.LBB4_24:
	leaq	alloc_c0960c38849cb512b20eefc3cdf9c036(%rip), %r8
	movl	$255, %ecx
	movl	$255, %edx
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_bounds_check
	ud2
	.seh_endproc

	.def	net$dns_names_internal$cxxbridge1$205$read_length;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",one_only,net$dns_names_internal$cxxbridge1$205$read_length,unique,5
	.globl	net$dns_names_internal$cxxbridge1$205$read_length
	.p2align	4
net$dns_names_internal$cxxbridge1$205$read_length:
.seh_proc net$dns_names_internal$cxxbridge1$205$read_length
	subq	$40, %rsp
	.seh_stackalloc 40
	.seh_endprologue
	movq	8(%rcx), %rax
	testq	%rax, %rax
	js	.LBB5_10
	movq	(%rcx), %rcx
	testb	%dl, %dl
	je	.LBB5_4
	cmpq	$2, %rax
	jb	.LBB5_9
	movzwl	(%rcx), %ecx
	rolw	$8, %cx
	movzwl	%cx, %ecx
	movl	$2, %edx
	subq	%rdx, %rax
	cmpq	%rax, %rcx
	ja	.LBB5_9
	jmp	.LBB5_8
.LBB5_4:
	testq	%rax, %rax
	je	.LBB5_9
	movzbl	(%rcx), %ecx
	movl	$1, %edx
	subq	%rdx, %rax
	cmpq	%rax, %rcx
	jbe	.LBB5_8
.LBB5_9:
	xorl	%eax, %eax
	.seh_startepilogue
	addq	$40, %rsp
	.seh_endepilogue
	retq
.LBB5_8:
	leal	(%rdx,%rcx,4), %eax
	.seh_startepilogue
	addq	$40, %rsp
	.seh_endepilogue
	retq
.LBB5_10:
	leaq	alloc_a28e8c8fd5088943a8b5d44af697ff83(%rip), %rcx
	leaq	alloc_7d52e238cba565908982a5cea838db34(%rip), %r9
	movl	$559, %edx
	xorl	%r8d, %r8d
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_nounwind_fmt
	ud2
	.seh_endproc

	.def	net$dns_names_internal$cxxbridge1$205$valid_name;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",one_only,net$dns_names_internal$cxxbridge1$205$valid_name,unique,6
	.globl	net$dns_names_internal$cxxbridge1$205$valid_name
	.p2align	4
net$dns_names_internal$cxxbridge1$205$valid_name:
.seh_proc net$dns_names_internal$cxxbridge1$205$valid_name
	subq	$40, %rsp
	.seh_stackalloc 40
	.seh_endprologue
	movq	8(%rcx), %rdx
	testq	%rdx, %rdx
	js	.LBB6_1
	movq	(%rcx), %rcx
	.seh_startepilogue
	addq	$40, %rsp
	.seh_endepilogue
	jmp	_RNvNtCs6ePvCSTRAf3_20dns_names_validation14dns_names_core5valid
.LBB6_1:
	leaq	alloc_a28e8c8fd5088943a8b5d44af697ff83(%rip), %rcx
	leaq	alloc_7d52e238cba565908982a5cea838db34(%rip), %r9
	movl	$559, %edx
	xorl	%r8d, %r8d
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_nounwind_fmt
	ud2
	.seh_endproc

	.def	net$dns_names_internal$cxxbridge1$205$valid_record;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",one_only,net$dns_names_internal$cxxbridge1$205$valid_record,unique,7
	.globl	net$dns_names_internal$cxxbridge1$205$valid_record
	.p2align	4
net$dns_names_internal$cxxbridge1$205$valid_record:
.seh_proc net$dns_names_internal$cxxbridge1$205$valid_record
	pushq	%rsi
	.seh_pushreg %rsi
	pushq	%rdi
	.seh_pushreg %rdi
	subq	$56, %rsp
	.seh_stackalloc 56
	.seh_endprologue
	movq	8(%rcx), %rsi
	testq	%rsi, %rsi
	js	.LBB7_9
	movq	(%rcx), %rdi
	movq	%rdi, %rcx
	movq	%rsi, %rdx
	callq	_RNvNtCs6ePvCSTRAf3_20dns_names_validation14dns_names_core5valid
	testb	%al, %al
	je	.LBB7_4
	movq	%rdi, 40(%rsp)
	movq	%rsi, 48(%rsp)
	leaq	40(%rsp), %rcx
	callq	net$dns_names_internal$cxxbridge1$205$host_localhost
	testb	%al, %al
	jne	.LBB7_4
	movq	%rdi, 40(%rsp)
	movq	%rsi, 48(%rsp)
	leaq	40(%rsp), %rcx
	callq	net$dns_names_internal$cxxbridge1$205$host_ip_literal
	testb	%al, %al
	je	.LBB7_8
.LBB7_4:
	xorl	%eax, %eax
	.seh_startepilogue
	addq	$56, %rsp
	popq	%rdi
	popq	%rsi
	.seh_endepilogue
	retq
.LBB7_8:
	movq	%rdi, 40(%rsp)
	movq	%rsi, 48(%rsp)
	leaq	40(%rsp), %rcx
	callq	net$dns_names_internal$cxxbridge1$205$host_url_address
	xorb	$1, %al
	.seh_startepilogue
	addq	$56, %rsp
	popq	%rdi
	popq	%rsi
	.seh_endepilogue
	retq
.LBB7_9:
	leaq	alloc_a28e8c8fd5088943a8b5d44af697ff83(%rip), %rcx
	leaq	alloc_7d52e238cba565908982a5cea838db34(%rip), %r9
	movl	$559, %edx
	xorl	%r8d, %r8d
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_nounwind_fmt
	ud2
	.seh_endproc

	.def	net_dns_names_less;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",one_only,net_dns_names_less,unique,8
	.globl	net_dns_names_less
	.p2align	4
net_dns_names_less:
.seh_proc net_dns_names_less
	pushq	%rsi
	.seh_pushreg %rsi
	pushq	%rdi
	.seh_pushreg %rdi
	pushq	%rbx
	.seh_pushreg %rbx
	subq	$32, %rsp
	.seh_stackalloc 32
	.seh_endprologue
	movl	$1, %eax
	movl	$1, %r10d
	testq	%rdx, %rdx
	je	.LBB8_4
	testq	%rcx, %rcx
	setne	%r11b
	testq	%rdx, %rdx
	setns	%bl
	movq	%rcx, %r10
	testb	%bl, %r11b
	je	.LBB8_2
.LBB8_4:
	testq	%r9, %r9
	je	.LBB8_7
	testq	%r8, %r8
	setne	%cl
	testq	%r9, %r9
	setns	%r11b
	movq	%r8, %rax
	testb	%r11b, %cl
	je	.LBB8_6
.LBB8_7:
	cmpq	%r9, %rdx
	movq	%r9, %rcx
	cmovbq	%rdx, %rcx
	testq	%rcx, %rcx
	je	.LBB8_11
	xorl	%r8d, %r8d
	jmp	.LBB8_9
	.p2align	4
.LBB8_10:
	incq	%r8
	cmpq	%r8, %rcx
	je	.LBB8_11
.LBB8_9:
	movzbl	(%r10,%r8), %esi
	movzbl	(%rax,%r8), %r11d
	cmpb	%r11b, %sil
	je	.LBB8_10
	leal	-65(%rsi), %edi
	cmpb	$26, %dil
	setb	%bl
	shlb	$5, %bl
	orb	%sil, %bl
	leal	-65(%r11), %esi
	cmpb	$26, %sil
	setb	%sil
	shlb	$5, %sil
	orb	%r11b, %sil
	cmpb	%sil, %bl
	je	.LBB8_10
	jmp	.LBB8_13
.LBB8_11:
	cmpq	%r9, %rdx
.LBB8_13:
	setb	%al
	.seh_startepilogue
	addq	$32, %rsp
	popq	%rbx
	popq	%rdi
	popq	%rsi
	.seh_endepilogue
	retq
.LBB8_2:
	leaq	alloc_a28e8c8fd5088943a8b5d44af697ff83(%rip), %rcx
	leaq	alloc_b500b2c6d4edc6a77f9cb49f7e5f8c1c(%rip), %r9
	movl	$559, %edx
	xorl	%r8d, %r8d
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_nounwind_fmt
	ud2
.LBB8_6:
	leaq	alloc_a28e8c8fd5088943a8b5d44af697ff83(%rip), %rcx
	leaq	alloc_32acf25e4c646ac316e9a5f2f8b83c43(%rip), %r9
	movl	$559, %edx
	xorl	%r8d, %r8d
	callq	_RNvNtCs9l4wcBNbII5_4core9panicking18panic_nounwind_fmt
	ud2
	.seh_endproc

	.section	.rdata,"dr",one_only,alloc_66307bfbaf09a97042d448f36116cf56,unique,9
alloc_66307bfbaf09a97042d448f36116cf56:
	.asciz	"D:\\Rustium\\net\\dns\\dns_names_core.rs"

	.section	.rdata,"dr",one_only,alloc_e8bce7c582003742c129b5a90af24fe0,unique,10
	.p2align	3, 0x0
alloc_e8bce7c582003742c129b5a90af24fe0:
	.quad	alloc_66307bfbaf09a97042d448f36116cf56
	.asciz	"$\000\000\000\000\000\000\000\217\000\000\000\t\000\000"

	.section	.rdata,"dr",one_only,alloc_c0960c38849cb512b20eefc3cdf9c036,unique,11
	.p2align	3, 0x0
alloc_c0960c38849cb512b20eefc3cdf9c036:
	.quad	alloc_66307bfbaf09a97042d448f36116cf56
	.asciz	"$\000\000\000\000\000\000\000\232\000\000\000\r\000\000"

	.section	.rdata,"dr",one_only,alloc_7420eb3f3eebbe4d2e8275b2ddfd7058,unique,12
	.p2align	3, 0x0
alloc_7420eb3f3eebbe4d2e8275b2ddfd7058:
	.quad	alloc_66307bfbaf09a97042d448f36116cf56
	.asciz	"$\000\000\000\000\000\000\000\235\000\000\000\017\000\000"

	.section	.rdata,"dr",one_only,alloc_95999eda6b30ca646b7466c393e2d425,unique,13
	.p2align	3, 0x0
alloc_95999eda6b30ca646b7466c393e2d425:
	.quad	alloc_66307bfbaf09a97042d448f36116cf56
	.asciz	"$\000\000\000\000\000\000\000%\000\000\000\016\000\000"

	.section	.rdata,"dr",one_only,alloc_936ea8349fd2f15fa257fdf99fc48b3a,unique,14
	.p2align	3, 0x0
alloc_936ea8349fd2f15fa257fdf99fc48b3a:
	.quad	alloc_66307bfbaf09a97042d448f36116cf56
	.asciz	"$\000\000\000\000\000\000\000\033\000\000\000$\000\000"

	.section	.rdata,"dr",one_only,alloc_45ee9b8e8fc13b44318480984e962740,unique,15
	.p2align	3, 0x0
alloc_45ee9b8e8fc13b44318480984e962740:
	.quad	alloc_66307bfbaf09a97042d448f36116cf56
	.asciz	"$\000\000\000\000\000\000\000\037\000\000\000\t\000\000"

	.section	.rdata,"dr",one_only,alloc_046d1a7a4881635cfad5b0c2ff7a23f6,unique,16
	.p2align	3, 0x0
alloc_046d1a7a4881635cfad5b0c2ff7a23f6:
	.quad	alloc_66307bfbaf09a97042d448f36116cf56
	.asciz	"$\000\000\000\000\000\000\000\036\000\000\000\034\000\000"

	.section	.rdata,"dr",one_only,alloc_1327df0da91bfd5afb36737253b9af79,unique,17
alloc_1327df0da91bfd5afb36737253b9af79:
	.asciz	"library\\core\\src\\ptr\\mod.rs"

	.section	.rdata,"dr",one_only,alloc_8b2b2beeccfb5776d8a78ad79e89f50b,unique,18
	.p2align	3, 0x0
alloc_8b2b2beeccfb5776d8a78ad79e89f50b:
	.quad	alloc_1327df0da91bfd5afb36737253b9af79
	.asciz	"\033\000\000\000\000\000\000\000&\002\000\000\005\000\000"

	.section	.rdata,"dr",one_only,alloc_87e6578268e354c4f2747b30d32a9b4e,unique,19
alloc_87e6578268e354c4f2747b30d32a9b4e:
	.asciz	"library\\core\\src\\slice\\iter.rs"

	.section	.rdata,"dr",one_only,alloc_1d1b1e35bb489427b2bdc9752de92226,unique,20
	.p2align	3, 0x0
alloc_1d1b1e35bb489427b2bdc9752de92226:
	.quad	alloc_87e6578268e354c4f2747b30d32a9b4e
	.asciz	"\036\000\000\000\000\000\000\000\214\000\000\000\001\000\000"

	.section	.rdata,"dr",one_only,alloc_af992eede0c6088ac8991e6615711c59,unique,21
alloc_af992eede0c6088ac8991e6615711c59:
	.asciz	"D:\\Rustium\\net\\dns\\dns_names_ffi.rs"

	.section	.rdata,"dr",one_only,alloc_1d9f78db3a4a8c034b111a5282e4d74b,unique,22
	.p2align	3, 0x0
alloc_1d9f78db3a4a8c034b111a5282e4d74b:
	.quad	alloc_af992eede0c6088ac8991e6615711c59
	.asciz	"#\000\000\000\000\000\000\000A\000\000\0003\000\000"

	.section	.rdata,"dr",one_only,alloc_bf8205ff82636365a7d0f6ecb946395f,unique,23
alloc_bf8205ff82636365a7d0f6ecb946395f:
	.asciz	"D:\\Rustium\\third_party\\rust\\chromium_crates_io\\vendor\\cxx-v1\\src\\rust_slice.rs"

	.section	.rdata,"dr",one_only,alloc_7d52e238cba565908982a5cea838db34,unique,24
	.p2align	3, 0x0
alloc_7d52e238cba565908982a5cea838db34:
	.quad	alloc_bf8205ff82636365a7d0f6ecb946395f
	.asciz	"N\000\000\000\000\000\000\000\035\000\000\000\022\000\000"

	.section	.rdata,"dr",one_only,alloc_bd3468a7b96187f70c1ce98a3e7a63bf,unique,25
alloc_bd3468a7b96187f70c1ce98a3e7a63bf:
	.ascii	"unsafe precondition(s) violated: ptr::copy_nonoverlapping requires that both pointer arguments are aligned and non-null and the specified memory ranges do not overlap\n\nThis indicates a bug in the program. This Undefined Behavior check is optional, and cannot be relied on for safety."

	.section	.rdata,"dr",one_only,alloc_64e308ef4babfeb8b6220184de794a17,unique,26
alloc_64e308ef4babfeb8b6220184de794a17:
	.ascii	"unsafe precondition(s) violated: hint::assert_unchecked must never be called when the condition is false\n\nThis indicates a bug in the program. This Undefined Behavior check is optional, and cannot be relied on for safety."

	.section	.rdata,"dr",one_only,alloc_a28e8c8fd5088943a8b5d44af697ff83,unique,27
alloc_a28e8c8fd5088943a8b5d44af697ff83:
	.ascii	"unsafe precondition(s) violated: slice::from_raw_parts requires the pointer to be aligned and non-null, and the total size of the slice not to exceed `isize::MAX`\n\nThis indicates a bug in the program. This Undefined Behavior check is optional, and cannot be relied on for safety."

	.section	.rdata,"dr",one_only,alloc_b500b2c6d4edc6a77f9cb49f7e5f8c1c,unique,28
	.p2align	3, 0x0
alloc_b500b2c6d4edc6a77f9cb49f7e5f8c1c:
	.quad	alloc_af992eede0c6088ac8991e6615711c59
	.asciz	"#\000\000\000\000\000\000\000\226\000\000\000\022\000\000"

	.section	.rdata,"dr",one_only,alloc_32acf25e4c646ac316e9a5f2f8b83c43,unique,29
	.p2align	3, 0x0
alloc_32acf25e4c646ac316e9a5f2f8b83c43:
	.quad	alloc_af992eede0c6088ac8991e6615711c59
	.asciz	"#\000\000\000\000\000\000\000\234\000\000\000\022\000\000"

	.section	.gfids$y,"dr"
	.symidx	__CxxFrameHandler3
	.section	.giats$y,"dr"
	.section	.gljmp$y,"dr"
