
D:\RustiumBuild\src\out\Default\obj\net\libhttp_range_parser_983ea169.rlib(lib.rmeta):	file format coff-x86-64

D:\RustiumBuild\src\out\Default\obj\net\libhttp_range_parser_983ea169.rlib(lib.rmeta-link):	file format coff-x86-64

D:\RustiumBuild\src\out\Default\obj\net\libhttp_range_parser_983ea169.rlib(libhttp_range_parser_983ea169.http_range_parser_983ea169.9b32271965b1abea-cgu.0.rcgu.o):	file format coff-x86-64

Disassembly of section .text:

0000000000000000 <http_range_parser_983ea169::bytes>:
       0: 48 83 ec 28                  	subq	$0x28, %rsp
       4: 48 85 d2                     	testq	%rdx, %rdx
       7: 74 1a                        	je	0x23 <http_range_parser_983ea169::bytes+0x23>
       9: 48 85 c9                     	testq	%rcx, %rcx
       c: 0f 95 c0                     	setne	%al
       f: 48 85 d2                     	testq	%rdx, %rdx
      12: 41 0f 99 c0                  	setns	%r8b
      16: 44 84 c0                     	testb	%r8b, %al
      19: 74 15                        	je	0x30 <http_range_parser_983ea169::bytes+0x30>
      1b: 48 89 c8                     	movq	%rcx, %rax
      1e: 48 83 c4 28                  	addq	$0x28, %rsp
      22: c3                           	retq
      23: b9 01 00 00 00               	movl	$0x1, %ecx
      28: 48 89 c8                     	movq	%rcx, %rax
      2b: 48 83 c4 28                  	addq	$0x28, %rsp
      2f: c3                           	retq
      30: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x37 <http_range_parser_983ea169::bytes+0x37>
      37: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x3e <http_range_parser_983ea169::bytes+0x3e>
      3e: ba 2f 02 00 00               	movl	$0x22f, %edx            # imm = 0x22F
      43: 45 31 c0                     	xorl	%r8d, %r8d
      46: e8 00 00 00 00               	callq	0x4b <http_range_parser_983ea169::bytes+0x4b>
      4b: 0f 0b                        	ud2

Disassembly of section .text$unlikely:

0000000000000000 <<alloc::raw_vec::RawVec<http_range_parser_983ea169::ParsedRange>>::grow_one>:
       0: 48 83 ec 28                  	subq	$0x28, %rsp
       4: 48 8b 11                     	movq	(%rcx), %rdx
       7: e8 00 00 00 00               	callq	0xc <<alloc::raw_vec::RawVec<http_range_parser_983ea169::ParsedRange>>::grow_one+0xc>
       c: 48 83 f8 ff                  	cmpq	$-0x1, %rax
      10: 75 05                        	jne	0x17 <<alloc::raw_vec::RawVec<http_range_parser_983ea169::ParsedRange>>::grow_one+0x17>
      12: 48 83 c4 28                  	addq	$0x28, %rsp
      16: c3                           	retq
      17: 48 89 c1                     	movq	%rax, %rcx
      1a: e8 00 00 00 00               	callq	0x1f <<alloc::raw_vec::RawVec<http_range_parser_983ea169::ParsedRange>>::grow_one+0x1f>
      1f: 0f 0b                        	ud2

Disassembly of section .text$unlikely:

0000000000000000 <<alloc::raw_vec::RawVecInner>::finish_grow>:
       0: 56                           	pushq	%rsi
       1: 57                           	pushq	%rdi
       2: 48 83 ec 38                  	subq	$0x38, %rsp
       6: 48 89 ce                     	movq	%rcx, %rsi
       9: 48 b8 55 55 55 55 55 55 55 05	movabsq	$0x555555555555555, %rax # imm = 0x555555555555555
      13: 49 39 c1                     	cmpq	%rax, %r9
      16: 76 12                        	jbe	0x2a <<alloc::raw_vec::RawVecInner>::finish_grow+0x2a>
      18: 48 c7 46 08 00 00 00 00      	movq	$0x0, 0x8(%rsi)
      20: b8 01 00 00 00               	movl	$0x1, %eax
      25: e9 bf 00 00 00               	jmp	0xe9 <<alloc::raw_vec::RawVecInner>::finish_grow+0xe9>
      2a: 48 85 d2                     	testq	%rdx, %rdx
      2d: 74 37                        	je	0x66 <<alloc::raw_vec::RawVecInner>::finish_grow+0x66>
      2f: 48 b9 ab aa aa aa aa aa aa 0a	movabsq	$0xaaaaaaaaaaaaaab, %rcx # imm = 0xAAAAAAAAAAAAAAB
      39: 48 39 ca                     	cmpq	%rcx, %rdx
      3c: 0f 83 b1 00 00 00            	jae	0xf3 <<alloc::raw_vec::RawVecInner>::finish_grow+0xf3>
      42: 48 ff c0                     	incq	%rax
      45: 48 39 c2                     	cmpq	%rax, %rdx
      48: 0f 83 c2 00 00 00            	jae	0x110 <<alloc::raw_vec::RawVecInner>::finish_grow+0x110>
      4e: 48 c1 e2 03                  	shlq	$0x3, %rdx
      52: 48 8d 04 52                  	leaq	(%rdx,%rdx,2), %rax
      56: 48 c7 44 24 28 08 00 00 00   	movq	$0x8, 0x28(%rsp)
      5f: 48 8d 4c 24 30               	leaq	0x30(%rsp), %rcx
      64: eb 07                        	jmp	0x6d <<alloc::raw_vec::RawVecInner>::finish_grow+0x6d>
      66: 48 8d 4c 24 28               	leaq	0x28(%rsp), %rcx
      6b: 31 c0                        	xorl	%eax, %eax
      6d: 49 c1 e1 03                  	shlq	$0x3, %r9
      71: 4b 8d 3c 49                  	leaq	(%r9,%r9,2), %rdi
      75: 48 89 01                     	movq	%rax, (%rcx)
      78: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
      7d: 48 85 c0                     	testq	%rax, %rax
      80: 74 46                        	je	0xc8 <<alloc::raw_vec::RawVecInner>::finish_grow+0xc8>
      82: 48 83 f8 08                  	cmpq	$0x8, %rax
      86: 0f 85 a1 00 00 00            	jne	0x12d <<alloc::raw_vec::RawVecInner>::finish_grow+0x12d>
      8c: 48 8b 54 24 30               	movq	0x30(%rsp), %rdx
      91: 48 39 d7                     	cmpq	%rdx, %rdi
      94: 0f 82 ad 00 00 00            	jb	0x147 <<alloc::raw_vec::RawVecInner>::finish_grow+0x147>
      9a: 48 85 d2                     	testq	%rdx, %rdx
      9d: 74 29                        	je	0xc8 <<alloc::raw_vec::RawVecInner>::finish_grow+0xc8>
      9f: 4c 89 c1                     	movq	%r8, %rcx
      a2: 41 b8 08 00 00 00            	movl	$0x8, %r8d
      a8: 49 89 f9                     	movq	%rdi, %r9
      ab: e8 00 00 00 00               	callq	0xb0 <<alloc::raw_vec::RawVecInner>::finish_grow+0xb0>
      b0: 48 85 c0                     	testq	%rax, %rax
      b3: 75 2a                        	jne	0xdf <<alloc::raw_vec::RawVecInner>::finish_grow+0xdf>
      b5: 48 c7 46 08 08 00 00 00      	movq	$0x8, 0x8(%rsi)
      bd: 48 89 7e 10                  	movq	%rdi, 0x10(%rsi)
      c1: b8 01 00 00 00               	movl	$0x1, %eax
      c6: eb 21                        	jmp	0xe9 <<alloc::raw_vec::RawVecInner>::finish_grow+0xe9>
      c8: e8 00 00 00 00               	callq	0xcd <<alloc::raw_vec::RawVecInner>::finish_grow+0xcd>
      cd: ba 08 00 00 00               	movl	$0x8, %edx
      d2: 48 89 f9                     	movq	%rdi, %rcx
      d5: e8 00 00 00 00               	callq	0xda <<alloc::raw_vec::RawVecInner>::finish_grow+0xda>
      da: 48 85 c0                     	testq	%rax, %rax
      dd: 74 d6                        	je	0xb5 <<alloc::raw_vec::RawVecInner>::finish_grow+0xb5>
      df: 48 89 46 08                  	movq	%rax, 0x8(%rsi)
      e3: 48 89 7e 10                  	movq	%rdi, 0x10(%rsi)
      e7: 31 c0                        	xorl	%eax, %eax
      e9: 48 89 06                     	movq	%rax, (%rsi)
      ec: 48 83 c4 38                  	addq	$0x38, %rsp
      f0: 5f                           	popq	%rdi
      f1: 5e                           	popq	%rsi
      f2: c3                           	retq
      f3: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xfa <<alloc::raw_vec::RawVecInner>::finish_grow+0xfa>
      fa: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x101 <<alloc::raw_vec::RawVecInner>::finish_grow+0x101>
     101: ba 75 01 00 00               	movl	$0x175, %edx            # imm = 0x175
     106: 45 31 c0                     	xorl	%r8d, %r8d
     109: e8 00 00 00 00               	callq	0x10e <<alloc::raw_vec::RawVecInner>::finish_grow+0x10e>
     10e: 0f 0b                        	ud2
     110: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x117 <<alloc::raw_vec::RawVecInner>::finish_grow+0x117>
     117: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x11e <<alloc::raw_vec::RawVecInner>::finish_grow+0x11e>
     11e: ba 33 02 00 00               	movl	$0x233, %edx            # imm = 0x233
     123: 45 31 c0                     	xorl	%r8d, %r8d
     126: e8 00 00 00 00               	callq	0x12b <<alloc::raw_vec::RawVecInner>::finish_grow+0x12b>
     12b: 0f 0b                        	ud2
     12d: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x134 <<alloc::raw_vec::RawVecInner>::finish_grow+0x134>
     134: 4c 8d 05 00 00 00 00         	leaq	(%rip), %r8             # 0x13b <<alloc::raw_vec::RawVecInner>::finish_grow+0x13b>
     13b: ba 3a 00 00 00               	movl	$0x3a, %edx
     140: e8 00 00 00 00               	callq	0x145 <<alloc::raw_vec::RawVecInner>::finish_grow+0x145>
     145: 0f 0b                        	ud2
     147: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x14e <<alloc::raw_vec::RawVecInner>::finish_grow+0x14e>
     14e: 4c 8d 05 00 00 00 00         	leaq	(%rip), %r8             # 0x155 <<alloc::raw_vec::RawVecInner>::finish_grow+0x155>
     155: ba 91 00 00 00               	movl	$0x91, %edx
     15a: e8 00 00 00 00               	callq	0x15f <<alloc::raw_vec::RawVecInner>::finish_grow+0x15f>
     15f: 0f 0b                        	ud2

Disassembly of section .text$unlikely:

0000000000000000 <<alloc::raw_vec::RawVecInner>::grow_amortized>:
       0: 56                           	pushq	%rsi
       1: 57                           	pushq	%rdi
       2: 48 83 ec 38                  	subq	$0x38, %rsp
       6: 48 89 ce                     	movq	%rcx, %rsi
       9: 48 ff c2                     	incq	%rdx
       c: 48 8b 01                     	movq	(%rcx), %rax
       f: 48 8d 0c 00                  	leaq	(%rax,%rax), %rcx
      13: 48 39 d1                     	cmpq	%rdx, %rcx
      16: 48 0f 47 d1                  	cmovaq	%rcx, %rdx
      1a: 48 83 fa 05                  	cmpq	$0x5, %rdx
      1e: bf 04 00 00 00               	movl	$0x4, %edi
      23: 48 0f 43 fa                  	cmovaeq	%rdx, %rdi
      27: 4c 8b 46 08                  	movq	0x8(%rsi), %r8
      2b: 48 8d 4c 24 20               	leaq	0x20(%rsp), %rcx
      30: 48 89 c2                     	movq	%rax, %rdx
      33: 49 89 f9                     	movq	%rdi, %r9
      36: e8 00 00 00 00               	callq	0x3b <<alloc::raw_vec::RawVecInner>::grow_amortized+0x3b>
      3b: 80 7c 24 20 00               	cmpb	$0x0, 0x20(%rsp)
      40: 74 11                        	je	0x53 <<alloc::raw_vec::RawVecInner>::grow_amortized+0x53>
      42: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
      47: 48 8b 54 24 30               	movq	0x30(%rsp), %rdx
      4c: 48 83 c4 38                  	addq	$0x38, %rsp
      50: 5f                           	popq	%rdi
      51: 5e                           	popq	%rsi
      52: c3                           	retq
      53: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
      58: 48 89 46 08                  	movq	%rax, 0x8(%rsi)
      5c: 48 89 3e                     	movq	%rdi, (%rsi)
      5f: 48 c7 c0 ff ff ff ff         	movq	$-0x1, %rax
      66: 48 83 c4 38                  	addq	$0x38, %rsp
      6a: 5f                           	popq	%rdi
      6b: 5e                           	popq	%rsi
      6c: c3                           	retq

Disassembly of section .text:

0000000000000000 <http_range_parser_983ea169::http_range_parser::legacy_i64>:
       0: 56                           	pushq	%rsi
       1: 48 83 ec 30                  	subq	$0x30, %rsp
       5: 48 85 d2                     	testq	%rdx, %rdx
       8: 0f 84 5e 01 00 00            	je	0x16c <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16c>
       e: 48 83 f9 ff                  	cmpq	$-0x1, %rcx
      12: 0f 84 fb 02 00 00            	je	0x313 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x313>
      18: 80 39 2b                     	cmpb	$0x2b, (%rcx)
      1b: 0f 85 92 00 00 00            	jne	0xb3 <http_range_parser_983ea169::http_range_parser::legacy_i64+0xb3>
      21: 49 89 d0                     	movq	%rdx, %r8
      24: 49 ff c8                     	decq	%r8
      27: 0f 84 3f 01 00 00            	je	0x16c <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16c>
      2d: 48 ff c1                     	incq	%rcx
      30: 0f b6 01                     	movzbl	(%rcx), %eax
      33: 04 d0                        	addb	$-0x30, %al
      35: 3c 09                        	cmpb	$0x9, %al
      37: 0f 87 2f 01 00 00            	ja	0x16c <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16c>
      3d: 48 83 fa 12                  	cmpq	$0x12, %rdx
      41: 72 2d                        	jb	0x70 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x70>
      43: 48 b8 30 30 30 30 30 30 30 30	movabsq	$0x3030303030303030, %rax # imm = 0x3030303030303030
      4d: 0f 1f 00                     	nopl	(%rax)
      50: 48 8b 11                     	movq	(%rcx), %rdx
      53: 48 31 c2                     	xorq	%rax, %rdx
      56: 4c 8b 49 08                  	movq	0x8(%rcx), %r9
      5a: 49 31 c1                     	xorq	%rax, %r9
      5d: 49 09 d1                     	orq	%rdx, %r9
      60: 75 0e                        	jne	0x70 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x70>
      62: 49 83 c0 f0                  	addq	$-0x10, %r8
      66: 48 83 c1 10                  	addq	$0x10, %rcx
      6a: 49 83 f8 10                  	cmpq	$0x10, %r8
      6e: 77 e0                        	ja	0x50 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x50>
      70: 49 89 c9                     	movq	%rcx, %r9
      73: 4d 01 c1                     	addq	%r8, %r9
      76: 0f 82 7a 02 00 00            	jb	0x2f6 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x2f6>
      7c: 4d 85 c0                     	testq	%r8, %r8
      7f: 0f 84 e7 00 00 00            	je	0x16c <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16c>
      85: 0f b6 11                     	movzbl	(%rcx), %edx
      88: 8d 42 d0                     	leal	-0x30(%rdx), %eax
      8b: 3c 09                        	cmpb	$0x9, %al
      8d: 0f 87 d9 00 00 00            	ja	0x16c <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16c>
      93: 80 fa 30                     	cmpb	$0x30, %dl
      96: 0f 85 85 01 00 00            	jne	0x221 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x221>
      9c: b8 01 00 00 00               	movl	$0x1, %eax
      a1: 49 83 f8 01                  	cmpq	$0x1, %r8
      a5: 0f 85 de 01 00 00            	jne	0x289 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x289>
      ab: 31 d2                        	xorl	%edx, %edx
      ad: 48 83 c4 30                  	addq	$0x30, %rsp
      b1: 5e                           	popq	%rsi
      b2: c3                           	retq
      b3: 48 83 fa 11                  	cmpq	$0x11, %rdx
      b7: 72 37                        	jb	0xf0 <http_range_parser_983ea169::http_range_parser::legacy_i64+0xf0>
      b9: 48 b8 30 30 30 30 30 30 30 30	movabsq	$0x3030303030303030, %rax # imm = 0x3030303030303030
      c3: 66 66 66 66 2e 0f 1f 84 00 00 00 00 00       	nopw	%cs:(%rax,%rax)
      d0: 4c 8b 01                     	movq	(%rcx), %r8
      d3: 49 31 c0                     	xorq	%rax, %r8
      d6: 4c 8b 49 08                  	movq	0x8(%rcx), %r9
      da: 49 31 c1                     	xorq	%rax, %r9
      dd: 4d 09 c1                     	orq	%r8, %r9
      e0: 75 0e                        	jne	0xf0 <http_range_parser_983ea169::http_range_parser::legacy_i64+0xf0>
      e2: 48 83 c2 f0                  	addq	$-0x10, %rdx
      e6: 48 83 c1 10                  	addq	$0x10, %rcx
      ea: 48 83 fa 10                  	cmpq	$0x10, %rdx
      ee: 77 e0                        	ja	0xd0 <http_range_parser_983ea169::http_range_parser::legacy_i64+0xd0>
      f0: 49 89 c8                     	movq	%rcx, %r8
      f3: 49 01 d0                     	addq	%rdx, %r8
      f6: 0f 82 fa 01 00 00            	jb	0x2f6 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x2f6>
      fc: 48 85 d2                     	testq	%rdx, %rdx
      ff: 74 6b                        	je	0x16c <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16c>
     101: 44 0f b6 09                  	movzbl	(%rcx), %r9d
     105: 41 8d 41 d0                  	leal	-0x30(%r9), %eax
     109: 3c 0a                        	cmpb	$0xa, %al
     10b: 73 0e                        	jae	0x11b <http_range_parser_983ea169::http_range_parser::legacy_i64+0x11b>
     10d: 48 ff c1                     	incq	%rcx
     110: 45 89 ca                     	movl	%r9d, %r10d
     113: 41 80 fa 30                  	cmpb	$0x30, %r10b
     117: 74 26                        	je	0x13f <http_range_parser_983ea169::http_range_parser::legacy_i64+0x13f>
     119: eb 47                        	jmp	0x162 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x162>
     11b: 41 80 f9 2d                  	cmpb	$0x2d, %r9b
     11f: 0f 94 c0                     	sete	%al
     122: 48 83 fa 01                  	cmpq	$0x1, %rdx
     126: 0f 95 c2                     	setne	%dl
     129: 20 c2                        	andb	%al, %dl
     12b: 80 fa 01                     	cmpb	$0x1, %dl
     12e: 75 3c                        	jne	0x16c <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16c>
     130: 44 0f b6 51 01               	movzbl	0x1(%rcx), %r10d
     135: 48 83 c1 02                  	addq	$0x2, %rcx
     139: 41 80 fa 30                  	cmpb	$0x30, %r10b
     13d: 75 23                        	jne	0x162 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x162>
     13f: b8 01 00 00 00               	movl	$0x1, %eax
     144: 31 d2                        	xorl	%edx, %edx
     146: 66 2e 0f 1f 84 00 00 00 00 00	nopw	%cs:(%rax,%rax)
     150: 4c 39 c1                     	cmpq	%r8, %rcx
     153: 74 19                        	je	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>
     155: 44 0f b6 11                  	movzbl	(%rcx), %r10d
     159: 48 ff c1                     	incq	%rcx
     15c: 41 80 fa 30                  	cmpb	$0x30, %r10b
     160: 74 ee                        	je	0x150 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x150>
     162: 41 80 c2 d0                  	addb	$-0x30, %r10b
     166: 41 80 fa 09                  	cmpb	$0x9, %r10b
     16a: 76 08                        	jbe	0x174 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x174>
     16c: 31 c0                        	xorl	%eax, %eax
     16e: 48 83 c4 30                  	addq	$0x30, %rsp
     172: 5e                           	popq	%rsi
     173: c3                           	retq
     174: 41 0f b6 d2                  	movzbl	%r10b, %edx
     178: 41 80 f9 2d                  	cmpb	$0x2d, %r9b
     17c: 75 4c                        	jne	0x1ca <http_range_parser_983ea169::http_range_parser::legacy_i64+0x1ca>
     17e: 4c 39 c1                     	cmpq	%r8, %rcx
     181: 0f 84 50 01 00 00            	je	0x2d7 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x2d7>
     187: 49 b9 cc cc cc cc cc cc cc 0c	movabsq	$0xccccccccccccccc, %r9 # imm = 0xCCCCCCCCCCCCCCC
     191: 31 c0                        	xorl	%eax, %eax
     193: eb 11                        	jmp	0x1a6 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x1a6>
     195: 48 8d 14 92                  	leaq	(%rdx,%rdx,4), %rdx
     199: 49 8d 14 52                  	leaq	(%r10,%rdx,2), %rdx
     19d: 4c 39 c1                     	cmpq	%r8, %rcx
     1a0: 0f 84 31 01 00 00            	je	0x2d7 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x2d7>
     1a6: 44 0f b6 11                  	movzbl	(%rcx), %r10d
     1aa: 41 80 c2 d0                  	addb	$-0x30, %r10b
     1ae: 41 80 fa 09                  	cmpb	$0x9, %r10b
     1b2: 77 ba                        	ja	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>
     1b4: 48 ff c1                     	incq	%rcx
     1b7: 4c 39 ca                     	cmpq	%r9, %rdx
     1ba: 77 b2                        	ja	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>
     1bc: 45 0f b6 d2                  	movzbl	%r10b, %r10d
     1c0: 75 d3                        	jne	0x195 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x195>
     1c2: 41 80 fa 09                  	cmpb	$0x9, %r10b
     1c6: 75 cd                        	jne	0x195 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x195>
     1c8: eb a4                        	jmp	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>
     1ca: 4c 39 c1                     	cmpq	%r8, %rcx
     1cd: 0f 84 07 01 00 00            	je	0x2da <http_range_parser_983ea169::http_range_parser::legacy_i64+0x2da>
     1d3: 49 b9 cc cc cc cc cc cc cc 0c	movabsq	$0xccccccccccccccc, %r9 # imm = 0xCCCCCCCCCCCCCCC
     1dd: 31 c0                        	xorl	%eax, %eax
     1df: eb 11                        	jmp	0x1f2 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x1f2>
     1e1: 48 8d 14 92                  	leaq	(%rdx,%rdx,4), %rdx
     1e5: 49 8d 14 52                  	leaq	(%r10,%rdx,2), %rdx
     1e9: 4c 39 c1                     	cmpq	%r8, %rcx
     1ec: 0f 84 e8 00 00 00            	je	0x2da <http_range_parser_983ea169::http_range_parser::legacy_i64+0x2da>
     1f2: 44 0f b6 11                  	movzbl	(%rcx), %r10d
     1f6: 41 80 c2 d0                  	addb	$-0x30, %r10b
     1fa: 41 80 fa 09                  	cmpb	$0x9, %r10b
     1fe: 0f 87 6a ff ff ff            	ja	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>
     204: 48 ff c1                     	incq	%rcx
     207: 4c 39 ca                     	cmpq	%r9, %rdx
     20a: 0f 87 5e ff ff ff            	ja	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>
     210: 45 0f b6 d2                  	movzbl	%r10b, %r10d
     214: 75 cb                        	jne	0x1e1 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x1e1>
     216: 41 80 fa 07                  	cmpb	$0x7, %r10b
     21a: 76 c5                        	jbe	0x1e1 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x1e1>
     21c: e9 4d ff ff ff               	jmp	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>
     221: 0f b6 d0                     	movzbl	%al, %edx
     224: b8 01 00 00 00               	movl	$0x1, %eax
     229: 49 83 f8 01                  	cmpq	$0x1, %r8
     22d: 0f 84 3b ff ff ff            	je	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>
     233: 48 ff c1                     	incq	%rcx
     236: 49 b8 cc cc cc cc cc cc cc 0c	movabsq	$0xccccccccccccccc, %r8 # imm = 0xCCCCCCCCCCCCCCC
     240: eb 16                        	jmp	0x258 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x258>
     242: 48 8d 14 92                  	leaq	(%rdx,%rdx,4), %rdx
     246: 48 8d 14 50                  	leaq	(%rax,%rdx,2), %rdx
     24a: b8 01 00 00 00               	movl	$0x1, %eax
     24f: 4c 39 c9                     	cmpq	%r9, %rcx
     252: 0f 84 16 ff ff ff            	je	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>
     258: 44 0f b6 11                  	movzbl	(%rcx), %r10d
     25c: 41 80 c2 d0                  	addb	$-0x30, %r10b
     260: 41 80 fa 09                  	cmpb	$0x9, %r10b
     264: 77 1b                        	ja	0x281 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x281>
     266: 48 ff c1                     	incq	%rcx
     269: b8 00 00 00 00               	movl	$0x0, %eax
     26e: 4c 39 c2                     	cmpq	%r8, %rdx
     271: 0f 87 f7 fe ff ff            	ja	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>
     277: 41 0f b6 c2                  	movzbl	%r10b, %eax
     27b: 75 c5                        	jne	0x242 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x242>
     27d: 3c 07                        	cmpb	$0x7, %al
     27f: 76 c1                        	jbe	0x242 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x242>
     281: 31 c0                        	xorl	%eax, %eax
     283: 48 83 c4 30                  	addq	$0x30, %rsp
     287: 5e                           	popq	%rsi
     288: c3                           	retq
     289: 48 89 54 24 28               	movq	%rdx, 0x28(%rsp)
     28e: 4c 89 c6                     	movq	%r8, %rsi
     291: 48 f7 de                     	negq	%rsi
     294: 41 ba 02 00 00 00            	movl	$0x2, %r10d
     29a: 31 d2                        	xorl	%edx, %edx
     29c: 46 0f b6 5c 11 ff            	movzbl	-0x1(%rcx,%r10), %r11d
     2a2: 41 80 c3 d0                  	addb	$-0x30, %r11b
     2a6: 75 15                        	jne	0x2bd <http_range_parser_983ea169::http_range_parser::legacy_i64+0x2bd>
     2a8: 4e 8d 1c 16                  	leaq	(%rsi,%r10), %r11
     2ac: 49 ff c3                     	incq	%r11
     2af: 49 ff c2                     	incq	%r10
     2b2: 49 83 fb 01                  	cmpq	$0x1, %r11
     2b6: 75 e4                        	jne	0x29c <http_range_parser_983ea169::http_range_parser::legacy_i64+0x29c>
     2b8: e9 b1 fe ff ff               	jmp	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>
     2bd: 41 80 fb 09                  	cmpb	$0x9, %r11b
     2c1: 0f 87 a5 fe ff ff            	ja	0x16c <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16c>
     2c7: 41 0f b6 d3                  	movzbl	%r11b, %edx
     2cb: 80 7c 24 28 2d               	cmpb	$0x2d, 0x28(%rsp)
     2d0: 75 13                        	jne	0x2e5 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x2e5>
     2d2: 4d 39 d0                     	cmpq	%r10, %r8
     2d5: 75 59                        	jne	0x330 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x330>
     2d7: 48 f7 da                     	negq	%rdx
     2da: b8 01 00 00 00               	movl	$0x1, %eax
     2df: 48 83 c4 30                  	addq	$0x30, %rsp
     2e3: 5e                           	popq	%rsi
     2e4: c3                           	retq
     2e5: 4d 39 d0                     	cmpq	%r10, %r8
     2e8: 0f 84 80 fe ff ff            	je	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>
     2ee: 4c 01 d1                     	addq	%r10, %rcx
     2f1: e9 40 ff ff ff               	jmp	0x236 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x236>
     2f6: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x2fd <http_range_parser_983ea169::http_range_parser::legacy_i64+0x2fd>
     2fd: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x304 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x304>
     304: ba ad 01 00 00               	movl	$0x1ad, %edx            # imm = 0x1AD
     309: 45 31 c0                     	xorl	%r8d, %r8d
     30c: e8 00 00 00 00               	callq	0x311 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x311>
     311: 0f 0b                        	ud2
     313: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x31a <http_range_parser_983ea169::http_range_parser::legacy_i64+0x31a>
     31a: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x321 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x321>
     321: ba ad 01 00 00               	movl	$0x1ad, %edx            # imm = 0x1AD
     326: 45 31 c0                     	xorl	%r8d, %r8d
     329: e8 00 00 00 00               	callq	0x32e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x32e>
     32e: 0f 0b                        	ud2
     330: 49 b9 cc cc cc cc cc cc cc 0c	movabsq	$0xccccccccccccccc, %r9 # imm = 0xCCCCCCCCCCCCCCC
     33a: 31 c0                        	xorl	%eax, %eax
     33c: eb 10                        	jmp	0x34e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x34e>
     33e: 48 8d 14 92                  	leaq	(%rdx,%rdx,4), %rdx
     342: 49 8d 14 53                  	leaq	(%r11,%rdx,2), %rdx
     346: 49 ff c2                     	incq	%r10
     349: 4d 39 d0                     	cmpq	%r10, %r8
     34c: 74 89                        	je	0x2d7 <http_range_parser_983ea169::http_range_parser::legacy_i64+0x2d7>
     34e: 46 0f b6 1c 11               	movzbl	(%rcx,%r10), %r11d
     353: 41 80 c3 d0                  	addb	$-0x30, %r11b
     357: 41 80 fb 09                  	cmpb	$0x9, %r11b
     35b: 0f 87 0d fe ff ff            	ja	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>
     361: 4c 39 ca                     	cmpq	%r9, %rdx
     364: 0f 87 04 fe ff ff            	ja	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>
     36a: 45 0f b6 db                  	movzbl	%r11b, %r11d
     36e: 75 ce                        	jne	0x33e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x33e>
     370: 41 80 fb 09                  	cmpb	$0x9, %r11b
     374: 75 c8                        	jne	0x33e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x33e>
     376: e9 f3 fd ff ff               	jmp	0x16e <http_range_parser_983ea169::http_range_parser::legacy_i64+0x16e>

Disassembly of section .text:

0000000000000000 <http_range_parser_983ea169::http_range_parser::fetch_decimal>:
       0: 56                           	pushq	%rsi
       1: 53                           	pushq	%rbx
       2: 48 83 ec 28                  	subq	$0x28, %rsp
       6: 4d 8b 10                     	movq	(%r8), %r10
       9: 49 39 d2                     	cmpq	%rdx, %r10
       c: 0f 83 d5 00 00 00            	jae	0xe7 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xe7>
      12: 4c 89 d0                     	movq	%r10, %rax
      15: 66 66 2e 0f 1f 84 00 00 00 00 00     	nopw	%cs:(%rax,%rax)
      20: 44 0f b6 0c 01               	movzbl	(%rcx,%rax), %r9d
      25: 41 80 c1 d0                  	addb	$-0x30, %r9b
      29: 41 80 f9 0a                  	cmpb	$0xa, %r9b
      2d: 0f 83 a5 00 00 00            	jae	0xd8 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xd8>
      33: 48 ff c0                     	incq	%rax
      36: 49 89 00                     	movq	%rax, (%r8)
      39: 48 39 c2                     	cmpq	%rax, %rdx
      3c: 75 e2                        	jne	0x20 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x20>
      3e: 49 89 d3                     	movq	%rdx, %r11
      41: 4c 89 d8                     	movq	%r11, %rax
      44: 4c 29 d0                     	subq	%r10, %rax
      47: 0f 84 9a 00 00 00            	je	0xe7 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xe7>
      4d: 41 0f 93 c0                  	setae	%r8b
      51: 49 39 d3                     	cmpq	%rdx, %r11
      54: 41 0f 96 c1                  	setbe	%r9b
      58: 45 84 c8                     	testb	%r9b, %r8b
      5b: 0f 84 63 01 00 00            	je	0x1c4 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x1c4>
      61: 4e 8d 0c 11                  	leaq	(%rcx,%r10), %r9
      65: 4c 01 c8                     	addq	%r9, %rax
      68: 0f 82 6d 01 00 00            	jb	0x1db <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x1db>
      6e: 48 ba ff ff ff ff ff ff ff 7f	movabsq	$0x7fffffffffffffff, %rdx # imm = 0x7FFFFFFFFFFFFFFF
      78: 45 0f b6 01                  	movzbl	(%r9), %r8d
      7c: 41 8d 70 d0                  	leal	-0x30(%r8), %esi
      80: b8 01 00 00 00               	movl	$0x1, %eax
      85: 40 80 fe 09                  	cmpb	$0x9, %sil
      89: 77 46                        	ja	0xd1 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xd1>
      8b: 41 80 f8 30                  	cmpb	$0x30, %r8b
      8f: 75 5f                        	jne	0xf0 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xf0>
      91: 4d 8d 4a 01                  	leaq	0x1(%r10), %r9
      95: 4d 39 d9                     	cmpq	%r11, %r9
      98: 74 35                        	je	0xcf <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xcf>
      9a: 49 83 c2 02                  	addq	$0x2, %r10
      9e: 4d 89 d9                     	movq	%r11, %r9
      a1: 49 f7 d9                     	negq	%r9
      a4: 66 66 66 2e 0f 1f 84 00 00 00 00 00  	nopw	%cs:(%rax,%rax)
      b0: 42 0f b6 5c 11 ff            	movzbl	-0x1(%rcx,%r10), %ebx
      b6: 80 c3 d0                     	addb	$-0x30, %bl
      b9: 0f 85 92 00 00 00            	jne	0x151 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x151>
      bf: 4b 8d 34 11                  	leaq	(%r9,%r10), %rsi
      c3: 48 ff c6                     	incq	%rsi
      c6: 49 ff c2                     	incq	%r10
      c9: 48 83 fe 01                  	cmpq	$0x1, %rsi
      cd: 75 e1                        	jne	0xb0 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xb0>
      cf: 31 d2                        	xorl	%edx, %edx
      d1: 48 83 c4 28                  	addq	$0x28, %rsp
      d5: 5b                           	popq	%rbx
      d6: 5e                           	popq	%rsi
      d7: c3                           	retq
      d8: 49 89 c3                     	movq	%rax, %r11
      db: 4c 89 d8                     	movq	%r11, %rax
      de: 4c 29 d0                     	subq	%r10, %rax
      e1: 0f 85 66 ff ff ff            	jne	0x4d <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x4d>
      e7: 31 c0                        	xorl	%eax, %eax
      e9: 48 83 c4 28                  	addq	$0x28, %rsp
      ed: 5b                           	popq	%rbx
      ee: 5e                           	popq	%rsi
      ef: c3                           	retq
      f0: 44 0f b6 c6                  	movzbl	%sil, %r8d
      f4: 49 ff c2                     	incq	%r10
      f7: 4d 39 da                     	cmpq	%r11, %r10
      fa: 75 0a                        	jne	0x106 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x106>
      fc: 4c 89 c2                     	movq	%r8, %rdx
      ff: 48 83 c4 28                  	addq	$0x28, %rsp
     103: 5b                           	popq	%rbx
     104: 5e                           	popq	%rsi
     105: c3                           	retq
     106: 49 ff c1                     	incq	%r9
     109: 4c 01 d9                     	addq	%r11, %rcx
     10c: 49 ba cc cc cc cc cc cc cc 0c	movabsq	$0xccccccccccccccc, %r10 # imm = 0xCCCCCCCCCCCCCCC
     116: eb 15                        	jmp	0x12d <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x12d>
     118: 0f 1f 84 00 00 00 00 00      	nopl	(%rax,%rax)
     120: 4f 8d 04 80                  	leaq	(%r8,%r8,4), %r8
     124: 4f 8d 04 43                  	leaq	(%r11,%r8,2), %r8
     128: 49 39 c9                     	cmpq	%rcx, %r9
     12b: 74 cf                        	je	0xfc <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xfc>
     12d: 45 0f b6 19                  	movzbl	(%r9), %r11d
     131: 41 80 c3 d0                  	addb	$-0x30, %r11b
     135: 41 80 fb 09                  	cmpb	$0x9, %r11b
     139: 77 96                        	ja	0xd1 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xd1>
     13b: 49 ff c1                     	incq	%r9
     13e: 4d 39 d0                     	cmpq	%r10, %r8
     141: 77 8e                        	ja	0xd1 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xd1>
     143: 45 0f b6 db                  	movzbl	%r11b, %r11d
     147: 75 d7                        	jne	0x120 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x120>
     149: 41 80 fb 07                  	cmpb	$0x7, %r11b
     14d: 76 d1                        	jbe	0x120 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x120>
     14f: eb 80                        	jmp	0xd1 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xd1>
     151: 80 fb 09                     	cmpb	$0x9, %bl
     154: 0f 87 77 ff ff ff            	ja	0xd1 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xd1>
     15a: 4d 89 c1                     	movq	%r8, %r9
     15d: 44 0f b6 c3                  	movzbl	%bl, %r8d
     161: 41 80 f9 2d                  	cmpb	$0x2d, %r9b
     165: 75 0a                        	jne	0x171 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x171>
     167: 4d 39 d3                     	cmpq	%r10, %r11
     16a: 75 12                        	jne	0x17e <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x17e>
     16c: 49 f7 d8                     	negq	%r8
     16f: eb 8b                        	jmp	0xfc <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xfc>
     171: 4d 39 d3                     	cmpq	%r10, %r11
     174: 74 86                        	je	0xfc <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xfc>
     176: 49 01 ca                     	addq	%rcx, %r10
     179: 4d 89 d1                     	movq	%r10, %r9
     17c: eb 8b                        	jmp	0x109 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x109>
     17e: 49 b9 cc cc cc cc cc cc cc 0c	movabsq	$0xccccccccccccccc, %r9 # imm = 0xCCCCCCCCCCCCCCC
     188: eb 10                        	jmp	0x19a <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x19a>
     18a: 4f 8d 04 80                  	leaq	(%r8,%r8,4), %r8
     18e: 4e 8d 04 46                  	leaq	(%rsi,%r8,2), %r8
     192: 49 ff c2                     	incq	%r10
     195: 4d 39 d3                     	cmpq	%r10, %r11
     198: 74 d2                        	je	0x16c <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x16c>
     19a: 42 0f b6 1c 11               	movzbl	(%rcx,%r10), %ebx
     19f: 80 c3 d0                     	addb	$-0x30, %bl
     1a2: 80 fb 09                     	cmpb	$0x9, %bl
     1a5: 0f 87 26 ff ff ff            	ja	0xd1 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xd1>
     1ab: 4d 39 c8                     	cmpq	%r9, %r8
     1ae: 0f 87 1d ff ff ff            	ja	0xd1 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xd1>
     1b4: 0f b6 f3                     	movzbl	%bl, %esi
     1b7: 75 d1                        	jne	0x18a <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x18a>
     1b9: 40 80 fe 09                  	cmpb	$0x9, %sil
     1bd: 75 cb                        	jne	0x18a <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x18a>
     1bf: e9 0d ff ff ff               	jmp	0xd1 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0xd1>
     1c4: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x1cb <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x1cb>
     1cb: 4c 89 d1                     	movq	%r10, %rcx
     1ce: 49 89 d0                     	movq	%rdx, %r8
     1d1: 4c 89 da                     	movq	%r11, %rdx
     1d4: e8 00 00 00 00               	callq	0x1d9 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x1d9>
     1d9: 0f 0b                        	ud2
     1db: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x1e2 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x1e2>
     1e2: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x1e9 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x1e9>
     1e9: ba ad 01 00 00               	movl	$0x1ad, %edx            # imm = 0x1AD
     1ee: 45 31 c0                     	xorl	%r8d, %r8d
     1f1: e8 00 00 00 00               	callq	0x1f6 <http_range_parser_983ea169::http_range_parser::fetch_decimal+0x1f6>
     1f6: 0f 0b                        	ud2

Disassembly of section .text$unlikely:

0000000000000000 <core::hint::unreachable_unchecked::precondition_check>:
       0: 48 83 ec 28                  	subq	$0x28, %rsp
       4: 49 89 c9                     	movq	%rcx, %r9
       7: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xe <core::hint::unreachable_unchecked::precondition_check+0xe>
       e: ba 8f 01 00 00               	movl	$0x18f, %edx            # imm = 0x18F
      13: 45 31 c0                     	xorl	%r8d, %r8d
      16: e8 00 00 00 00               	callq	0x1b <core::hint::unreachable_unchecked::precondition_check+0x1b>
      1b: 0f 0b                        	ud2

Disassembly of section .text:

0000000000000000 <rustium_http_range_content>:
       0: 41 57                        	pushq	%r15
       2: 41 56                        	pushq	%r14
       4: 41 55                        	pushq	%r13
       6: 41 54                        	pushq	%r12
       8: 56                           	pushq	%rsi
       9: 57                           	pushq	%rdi
       a: 55                           	pushq	%rbp
       b: 53                           	pushq	%rbx
       c: 48 83 ec 78                  	subq	$0x78, %rsp
      10: 4c 8b 9c 24 e0 00 00 00      	movq	0xe0(%rsp), %r11
      18: 41 f6 c3 07                  	testb	$0x7, %r11b
      1c: 0f 94 c0                     	sete	%al
      1f: 4d 85 db                     	testq	%r11, %r11
      22: 41 0f 95 c2                  	setne	%r10b
      26: 41 84 c2                     	testb	%al, %r10b
      29: 0f 84 48 03 00 00            	je	0x377 <rustium_http_range_content+0x377>
      2f: 49 c7 03 ff ff ff ff         	movq	$-0x1, (%r11)
      36: 41 f6 c1 07                  	testb	$0x7, %r9b
      3a: 0f 95 c0                     	setne	%al
      3d: 4d 85 c9                     	testq	%r9, %r9
      40: 41 0f 94 c2                  	sete	%r10b
      44: 41 08 c2                     	orb	%al, %r10b
      47: 0f 85 2a 03 00 00            	jne	0x377 <rustium_http_range_content+0x377>
      4d: 49 c7 01 ff ff ff ff         	movq	$-0x1, (%r9)
      54: 41 f6 c0 07                  	testb	$0x7, %r8b
      58: 0f 95 c0                     	setne	%al
      5b: 4d 85 c0                     	testq	%r8, %r8
      5e: 41 0f 94 c2                  	sete	%r10b
      62: 41 08 c2                     	orb	%al, %r10b
      65: 0f 85 0c 03 00 00            	jne	0x377 <rustium_http_range_content+0x377>
      6b: 49 c7 00 ff ff ff ff         	movq	$-0x1, (%r8)
      72: 48 85 d2                     	testq	%rdx, %rdx
      75: 0f 84 96 00 00 00            	je	0x111 <rustium_http_range_content+0x111>
      7b: 48 85 c9                     	testq	%rcx, %rcx
      7e: 0f 95 c0                     	setne	%al
      81: 48 85 d2                     	testq	%rdx, %rdx
      84: 41 0f 99 c2                  	setns	%r10b
      88: 44 84 d0                     	testb	%r10b, %al
      8b: 0f 84 fa 04 00 00            	je	0x58b <rustium_http_range_content+0x58b>
      91: 48 89 c8                     	movq	%rcx, %rax
      94: 48 01 d0                     	addq	%rdx, %rax
      97: 0f 82 1f 04 00 00            	jb	0x4bc <rustium_http_range_content+0x4bc>
      9d: 31 f6                        	xorl	%esi, %esi
      9f: eb 17                        	jmp	0xb8 <rustium_http_range_content+0xb8>
      a1: 66 66 66 66 66 66 2e 0f 1f 84 00 00 00 00 00 	nopw	%cs:(%rax,%rax)
      b0: 48 ff c6                     	incq	%rsi
      b3: 48 39 f2                     	cmpq	%rsi, %rdx
      b6: 74 18                        	je	0xd0 <rustium_http_range_content+0xd0>
      b8: 0f b6 04 31                  	movzbl	(%rcx,%rsi), %eax
      bc: 83 f8 09                     	cmpl	$0x9, %eax
      bf: 74 ef                        	je	0xb0 <rustium_http_range_content+0xb0>
      c1: 83 f8 20                     	cmpl	$0x20, %eax
      c4: 74 ea                        	je	0xb0 <rustium_http_range_content+0xb0>
      c6: 48 39 d6                     	cmpq	%rdx, %rsi
      c9: 72 08                        	jb	0xd3 <rustium_http_range_content+0xd3>
      cb: e9 3a 05 00 00               	jmp	0x60a <rustium_http_range_content+0x60a>
      d0: 48 89 d6                     	movq	%rdx, %rsi
      d3: 48 89 d7                     	movq	%rdx, %rdi
      d6: 48 89 d0                     	movq	%rdx, %rax
      d9: eb 0d                        	jmp	0xe8 <rustium_http_range_content+0xe8>
      db: 0f 1f 44 00 00               	nopl	(%rax,%rax)
      e0: 48 89 c7                     	movq	%rax, %rdi
      e3: 48 85 c0                     	testq	%rax, %rax
      e6: 74 21                        	je	0x109 <rustium_http_range_content+0x109>
      e8: 48 83 e8 01                  	subq	$0x1, %rax
      ec: 0f 82 fe 05 00 00            	jb	0x6f0 <rustium_http_range_content+0x6f0>
      f2: 44 0f b6 54 39 ff            	movzbl	-0x1(%rcx,%rdi), %r10d
      f8: 41 83 fa 20                  	cmpl	$0x20, %r10d
      fc: 74 e2                        	je	0xe0 <rustium_http_range_content+0xe0>
      fe: 41 83 fa 09                  	cmpl	$0x9, %r10d
     102: 74 dc                        	je	0xe0 <rustium_http_range_content+0xe0>
     104: 49 89 cb                     	movq	%rcx, %r11
     107: eb 12                        	jmp	0x11b <rustium_http_range_content+0x11b>
     109: 49 89 cb                     	movq	%rcx, %r11
     10c: 48 89 f7                     	movq	%rsi, %rdi
     10f: eb 0a                        	jmp	0x11b <rustium_http_range_content+0x11b>
     111: 41 bb 01 00 00 00            	movl	$0x1, %r11d
     117: 31 f6                        	xorl	%esi, %esi
     119: 31 ff                        	xorl	%edi, %edi
     11b: 49 89 fc                     	movq	%rdi, %r12
     11e: 49 29 f4                     	subq	%rsi, %r12
     121: 0f 93 c0                     	setae	%al
     124: 48 39 d7                     	cmpq	%rdx, %rdi
     127: 41 0f 96 c2                  	setbe	%r10b
     12b: 44 84 d0                     	testb	%r10b, %al
     12e: 0f 84 60 02 00 00            	je	0x394 <rustium_http_range_content+0x394>
     134: 4d 8d 34 33                  	leaq	(%r11,%rsi), %r14
     138: 4c 89 f0                     	movq	%r14, %rax
     13b: 48 f7 d0                     	notq	%rax
     13e: 49 83 fc 11                  	cmpq	$0x11, %r12
     142: 73 30                        	jae	0x174 <rustium_http_range_content+0x174>
     144: 49 39 c4                     	cmpq	%rax, %r12
     147: 0f 87 6f 03 00 00            	ja	0x4bc <rustium_http_range_content+0x4bc>
     14d: 48 39 f7                     	cmpq	%rsi, %rdi
     150: 0f 84 d5 06 00 00            	je	0x82b <rustium_http_range_content+0x82b>
     156: 4c 89 5c 24 40               	movq	%r11, 0x40(%rsp)
     15b: 45 31 db                     	xorl	%r11d, %r11d
     15e: 66 90                        	nop
     160: 43 80 3c 1e 20               	cmpb	$0x20, (%r14,%r11)
     165: 74 75                        	je	0x1dc <rustium_http_range_content+0x1dc>
     167: 49 ff c3                     	incq	%r11
     16a: 4d 39 dc                     	cmpq	%r11, %r12
     16d: 75 f1                        	jne	0x160 <rustium_http_range_content+0x160>
     16f: e9 b7 06 00 00               	jmp	0x82b <rustium_http_range_content+0x82b>
     174: 49 39 c4                     	cmpq	%rax, %r12
     177: 0f 87 8e 03 00 00            	ja	0x50b <rustium_http_range_content+0x50b>
     17d: 49 89 d5                     	movq	%rdx, %r13
     180: 49 89 cf                     	movq	%rcx, %r15
     183: 4c 89 cd                     	movq	%r9, %rbp
     186: 4c 89 c3                     	movq	%r8, %rbx
     189: 4c 89 5c 24 40               	movq	%r11, 0x40(%rsp)
     18e: 4d 8d 04 3b                  	leaq	(%r11,%rdi), %r8
     192: 48 8b 05 00 00 00 00         	movq	(%rip), %rax            # 0x199 <rustium_http_range_content+0x199>
     199: 48 8b 00                     	movq	(%rax), %rax
     19c: b1 20                        	movb	$0x20, %cl
     19e: 4c 89 f2                     	movq	%r14, %rdx
     1a1: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x1a7 <rustium_http_range_content+0x1a7>
     1a7: 48 83 f8 01                  	cmpq	$0x1, %rax
     1ab: 0f 85 7a 06 00 00            	jne	0x82b <rustium_http_range_content+0x82b>
     1b1: 49 89 d3                     	movq	%rdx, %r11
     1b4: 4d 29 f3                     	subq	%r14, %r11
     1b7: 0f 88 6b 03 00 00            	js	0x528 <rustium_http_range_content+0x528>
     1bd: 4d 39 e3                     	cmpq	%r12, %r11
     1c0: 49 89 d8                     	movq	%rbx, %r8
     1c3: 49 89 e9                     	movq	%rbp, %r9
     1c6: 4c 89 f9                     	movq	%r15, %rcx
     1c9: 4c 89 ea                     	movq	%r13, %rdx
     1cc: 72 17                        	jb	0x1e5 <rustium_http_range_content+0x1e5>
     1ce: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x1d5 <rustium_http_range_content+0x1d5>
     1d5: e8 00 00 00 00               	callq	0x1da <rustium_http_range_content+0x1da>
     1da: 0f 0b                        	ud2
     1dc: 4d 39 e3                     	cmpq	%r12, %r11
     1df: 0f 83 25 04 00 00            	jae	0x60a <rustium_http_range_content+0x60a>
     1e5: 4d 8d 14 33                  	leaq	(%r11,%rsi), %r10
     1e9: 49 39 d2                     	cmpq	%rdx, %r10
     1ec: 0f 87 25 02 00 00            	ja	0x417 <rustium_http_range_content+0x417>
     1f2: 4d 85 db                     	testq	%r11, %r11
     1f5: 0f 84 30 06 00 00            	je	0x82b <rustium_http_range_content+0x82b>
     1fb: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
     200: 48 8d 1c 06                  	leaq	(%rsi,%rax), %rbx
     204: 48 83 c3 04                  	addq	$0x4, %rbx
     208: 31 c0                        	xorl	%eax, %eax
     20a: eb 10                        	jmp	0x21c <rustium_http_range_content+0x21c>
     20c: 0f 1f 40 00                  	nopl	(%rax)
     210: 48 ff c0                     	incq	%rax
     213: 49 39 c3                     	cmpq	%rax, %r11
     216: 0f 84 0f 06 00 00            	je	0x82b <rustium_http_range_content+0x82b>
     21c: 44 0f b6 6c 03 fc            	movzbl	-0x4(%rbx,%rax), %r13d
     222: 41 83 fd 09                  	cmpl	$0x9, %r13d
     226: 74 e8                        	je	0x210 <rustium_http_range_content+0x210>
     228: 41 83 fd 20                  	cmpl	$0x20, %r13d
     22c: 74 e2                        	je	0x210 <rustium_http_range_content+0x210>
     22e: 48 8b 5c 24 40               	movq	0x40(%rsp), %rbx
     233: 4e 8d 3c 13                  	leaq	(%rbx,%r10), %r15
     237: 49 ff cf                     	decq	%r15
     23a: 31 ed                        	xorl	%ebp, %ebp
     23c: eb 15                        	jmp	0x253 <rustium_http_range_content+0x253>
     23e: 66 90                        	nop
     240: 49 8d 1c 2b                  	leaq	(%r11,%rbp), %rbx
     244: 48 ff cb                     	decq	%rbx
     247: 48 ff cd                     	decq	%rbp
     24a: 48 39 c3                     	cmpq	%rax, %rbx
     24d: 0f 84 d8 05 00 00            	je	0x82b <rustium_http_range_content+0x82b>
     253: 41 0f b6 1c 2f               	movzbl	(%r15,%rbp), %ebx
     258: 83 fb 20                     	cmpl	$0x20, %ebx
     25b: 74 e3                        	je	0x240 <rustium_http_range_content+0x240>
     25d: 83 fb 09                     	cmpl	$0x9, %ebx
     260: 74 de                        	je	0x240 <rustium_http_range_content+0x240>
     262: 49 8d 1c 2b                  	leaq	(%r11,%rbp), %rbx
     266: 48 83 c3 fb                  	addq	$-0x5, %rbx
     26a: 48 39 c3                     	cmpq	%rax, %rbx
     26d: 0f 85 b8 05 00 00            	jne	0x82b <rustium_http_range_content+0x82b>
     273: 41 8d 5d bf                  	leal	-0x41(%r13), %ebx
     277: 80 fb 1a                     	cmpb	$0x1a, %bl
     27a: 0f 92 c3                     	setb	%bl
     27d: c0 e3 05                     	shlb	$0x5, %bl
     280: 44 08 eb                     	orb	%r13b, %bl
     283: 80 fb 62                     	cmpb	$0x62, %bl
     286: 0f 85 9f 05 00 00            	jne	0x82b <rustium_http_range_content+0x82b>
     28c: 42 0f b6 5c 30 01            	movzbl	0x1(%rax,%r14), %ebx
     292: 8d 6b bf                     	leal	-0x41(%rbx), %ebp
     295: 40 80 fd 1a                  	cmpb	$0x1a, %bpl
     299: 40 0f 92 c5                  	setb	%bpl
     29d: 40 c0 e5 05                  	shlb	$0x5, %bpl
     2a1: 40 08 dd                     	orb	%bl, %bpl
     2a4: 40 80 fd 79                  	cmpb	$0x79, %bpl
     2a8: 0f 85 7d 05 00 00            	jne	0x82b <rustium_http_range_content+0x82b>
     2ae: 42 0f b6 5c 30 02            	movzbl	0x2(%rax,%r14), %ebx
     2b4: 8d 6b bf                     	leal	-0x41(%rbx), %ebp
     2b7: 40 80 fd 1a                  	cmpb	$0x1a, %bpl
     2bb: 40 0f 92 c5                  	setb	%bpl
     2bf: 40 c0 e5 05                  	shlb	$0x5, %bpl
     2c3: 40 08 dd                     	orb	%bl, %bpl
     2c6: 40 80 fd 74                  	cmpb	$0x74, %bpl
     2ca: 0f 85 5b 05 00 00            	jne	0x82b <rustium_http_range_content+0x82b>
     2d0: 42 0f b6 5c 30 03            	movzbl	0x3(%rax,%r14), %ebx
     2d6: 8d 6b bf                     	leal	-0x41(%rbx), %ebp
     2d9: 40 80 fd 1a                  	cmpb	$0x1a, %bpl
     2dd: 40 0f 92 c5                  	setb	%bpl
     2e1: 40 c0 e5 05                  	shlb	$0x5, %bpl
     2e5: 40 08 dd                     	orb	%bl, %bpl
     2e8: 40 80 fd 65                  	cmpb	$0x65, %bpl
     2ec: 0f 85 39 05 00 00            	jne	0x82b <rustium_http_range_content+0x82b>
     2f2: 42 0f b6 44 30 04            	movzbl	0x4(%rax,%r14), %eax
     2f8: 8d 58 bf                     	leal	-0x41(%rax), %ebx
     2fb: 80 fb 1a                     	cmpb	$0x1a, %bl
     2fe: 0f 92 c3                     	setb	%bl
     301: c0 e3 05                     	shlb	$0x5, %bl
     304: 08 c3                        	orb	%al, %bl
     306: 80 fb 73                     	cmpb	$0x73, %bl
     309: 0f 85 1c 05 00 00            	jne	0x82b <rustium_http_range_content+0x82b>
     30f: 4c 89 5c 24 70               	movq	%r11, 0x70(%rsp)
     314: 4d 8d 5a 01                  	leaq	0x1(%r10), %r11
     318: 4c 39 d7                     	cmpq	%r10, %rdi
     31b: 0f 86 1d 05 00 00            	jbe	0x83e <rustium_http_range_content+0x83e>
     321: 48 89 f8                     	movq	%rdi, %rax
     324: 4c 29 d8                     	subq	%r11, %rax
     327: 48 8b 5c 24 40               	movq	0x40(%rsp), %rbx
     32c: 4e 8d 34 1b                  	leaq	(%rbx,%r11), %r14
     330: 4c 89 f3                     	movq	%r14, %rbx
     333: 48 f7 d3                     	notq	%rbx
     336: 49 89 c5                     	movq	%rax, %r13
     339: 48 83 f8 11                  	cmpq	$0x11, %rax
     33d: 73 6c                        	jae	0x3ab <rustium_http_range_content+0x3ab>
     33f: 49 39 dd                     	cmpq	%rbx, %r13
     342: 0f 87 74 01 00 00            	ja	0x4bc <rustium_http_range_content+0x4bc>
     348: 4c 39 df                     	cmpq	%r11, %rdi
     34b: 0f 84 da 04 00 00            	je	0x82b <rustium_http_range_content+0x82b>
     351: 48 89 54 24 38               	movq	%rdx, 0x38(%rsp)
     356: 48 89 4c 24 28               	movq	%rcx, 0x28(%rsp)
     35b: 31 d2                        	xorl	%edx, %edx
     35d: 41 80 3c 16 2d               	cmpb	$0x2d, (%r14,%rdx)
     362: 0f 84 c6 00 00 00            	je	0x42e <rustium_http_range_content+0x42e>
     368: 48 ff c2                     	incq	%rdx
     36b: 31 c0                        	xorl	%eax, %eax
     36d: 49 39 d5                     	cmpq	%rdx, %r13
     370: 75 eb                        	jne	0x35d <rustium_http_range_content+0x35d>
     372: e9 b6 04 00 00               	jmp	0x82d <rustium_http_range_content+0x82d>
     377: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x37e <rustium_http_range_content+0x37e>
     37e: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x385 <rustium_http_range_content+0x385>
     385: ba b7 01 00 00               	movl	$0x1b7, %edx            # imm = 0x1B7
     38a: 45 31 c0                     	xorl	%r8d, %r8d
     38d: e8 00 00 00 00               	callq	0x392 <rustium_http_range_content+0x392>
     392: 0f 0b                        	ud2
     394: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x39b <rustium_http_range_content+0x39b>
     39b: 48 89 f1                     	movq	%rsi, %rcx
     39e: 49 89 d0                     	movq	%rdx, %r8
     3a1: 48 89 fa                     	movq	%rdi, %rdx
     3a4: e8 00 00 00 00               	callq	0x3a9 <rustium_http_range_content+0x3a9>
     3a9: 0f 0b                        	ud2
     3ab: 4c 89 5c 24 50               	movq	%r11, 0x50(%rsp)
     3b0: 4c 89 54 24 68               	movq	%r10, 0x68(%rsp)
     3b5: 48 89 54 24 38               	movq	%rdx, 0x38(%rsp)
     3ba: 48 89 4c 24 28               	movq	%rcx, 0x28(%rsp)
     3bf: 4c 89 4c 24 60               	movq	%r9, 0x60(%rsp)
     3c4: 4c 89 44 24 48               	movq	%r8, 0x48(%rsp)
     3c9: 49 39 dd                     	cmpq	%rbx, %r13
     3cc: 0f 87 39 01 00 00            	ja	0x50b <rustium_http_range_content+0x50b>
     3d2: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
     3d7: 4c 8d 04 38                  	leaq	(%rax,%rdi), %r8
     3db: 48 8b 05 00 00 00 00         	movq	(%rip), %rax            # 0x3e2 <rustium_http_range_content+0x3e2>
     3e2: 48 8b 00                     	movq	(%rax), %rax
     3e5: b1 2d                        	movb	$0x2d, %cl
     3e7: 4c 89 f2                     	movq	%r14, %rdx
     3ea: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x3f0 <rustium_http_range_content+0x3f0>
     3f0: 48 83 f8 01                  	cmpq	$0x1, %rax
     3f4: 0f 85 31 04 00 00            	jne	0x82b <rustium_http_range_content+0x82b>
     3fa: 4c 29 f2                     	subq	%r14, %rdx
     3fd: 0f 88 25 01 00 00            	js	0x528 <rustium_http_range_content+0x528>
     403: 4c 39 ea                     	cmpq	%r13, %rdx
     406: 4c 8b 44 24 68               	movq	0x68(%rsp), %r8
     40b: 4c 8b 4c 24 50               	movq	0x50(%rsp), %r9
     410: 72 35                        	jb	0x447 <rustium_http_range_content+0x447>
     412: e9 b7 fd ff ff               	jmp	0x1ce <rustium_http_range_content+0x1ce>
     417: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x41e <rustium_http_range_content+0x41e>
     41e: 48 89 f1                     	movq	%rsi, %rcx
     421: 49 89 d0                     	movq	%rdx, %r8
     424: 4c 89 d2                     	movq	%r10, %rdx
     427: e8 00 00 00 00               	callq	0x42c <rustium_http_range_content+0x42c>
     42c: 0f 0b                        	ud2
     42e: 4c 89 4c 24 60               	movq	%r9, 0x60(%rsp)
     433: 4c 89 44 24 48               	movq	%r8, 0x48(%rsp)
     438: 4c 39 ea                     	cmpq	%r13, %rdx
     43b: 4d 89 d0                     	movq	%r10, %r8
     43e: 4d 89 d9                     	movq	%r11, %r9
     441: 0f 83 c3 01 00 00            	jae	0x60a <rustium_http_range_content+0x60a>
     447: 49 01 d1                     	addq	%rdx, %r9
     44a: 4d 39 c1                     	cmpq	%r8, %r9
     44d: 4c 8b 5c 24 40               	movq	0x40(%rsp), %r11
     452: 0f 86 37 04 00 00            	jbe	0x88f <rustium_http_range_content+0x88f>
     458: 49 8d 49 01                  	leaq	0x1(%r9), %rcx
     45c: 4c 39 cf                     	cmpq	%r9, %rdi
     45f: 0f 86 f0 03 00 00            	jbe	0x855 <rustium_http_range_content+0x855>
     465: 48 89 f8                     	movq	%rdi, %rax
     468: 48 29 c8                     	subq	%rcx, %rax
     46b: 4d 8d 34 0b                  	leaq	(%r11,%rcx), %r14
     46f: 4d 89 f2                     	movq	%r14, %r10
     472: 49 f7 d2                     	notq	%r10
     475: 48 83 f8 11                  	cmpq	$0x11, %rax
     479: 48 89 54 24 58               	movq	%rdx, 0x58(%rsp)
     47e: 48 89 4c 24 30               	movq	%rcx, 0x30(%rsp)
     483: 4c 89 44 24 68               	movq	%r8, 0x68(%rsp)
     488: 4c 89 4c 24 50               	movq	%r9, 0x50(%rsp)
     48d: 73 3d                        	jae	0x4cc <rustium_http_range_content+0x4cc>
     48f: 4c 39 d0                     	cmpq	%r10, %rax
     492: 77 28                        	ja	0x4bc <rustium_http_range_content+0x4bc>
     494: 48 39 cf                     	cmpq	%rcx, %rdi
     497: 0f 84 8e 03 00 00            	je	0x82b <rustium_http_range_content+0x82b>
     49d: 31 db                        	xorl	%ebx, %ebx
     49f: 48 89 c1                     	movq	%rax, %rcx
     4a2: 41 80 3c 1e 2f               	cmpb	$0x2f, (%r14,%rbx)
     4a7: 0f 84 89 00 00 00            	je	0x536 <rustium_http_range_content+0x536>
     4ad: 48 ff c3                     	incq	%rbx
     4b0: 31 c0                        	xorl	%eax, %eax
     4b2: 48 39 d9                     	cmpq	%rbx, %rcx
     4b5: 75 eb                        	jne	0x4a2 <rustium_http_range_content+0x4a2>
     4b7: e9 71 03 00 00               	jmp	0x82d <rustium_http_range_content+0x82d>
     4bc: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x4c3 <rustium_http_range_content+0x4c3>
     4c3: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x4ca <rustium_http_range_content+0x4ca>
     4ca: eb 4d                        	jmp	0x519 <rustium_http_range_content+0x519>
     4cc: 4c 39 d0                     	cmpq	%r10, %rax
     4cf: 77 3a                        	ja	0x50b <rustium_http_range_content+0x50b>
     4d1: 49 89 c5                     	movq	%rax, %r13
     4d4: 49 01 fb                     	addq	%rdi, %r11
     4d7: 48 8b 05 00 00 00 00         	movq	(%rip), %rax            # 0x4de <rustium_http_range_content+0x4de>
     4de: 48 8b 00                     	movq	(%rax), %rax
     4e1: b1 2f                        	movb	$0x2f, %cl
     4e3: 4c 89 f2                     	movq	%r14, %rdx
     4e6: 4d 89 d8                     	movq	%r11, %r8
     4e9: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x4ef <rustium_http_range_content+0x4ef>
     4ef: 48 83 f8 01                  	cmpq	$0x1, %rax
     4f3: 0f 85 32 03 00 00            	jne	0x82b <rustium_http_range_content+0x82b>
     4f9: 48 89 d3                     	movq	%rdx, %rbx
     4fc: 4c 29 f3                     	subq	%r14, %rbx
     4ff: 78 27                        	js	0x528 <rustium_http_range_content+0x528>
     501: 4c 39 eb                     	cmpq	%r13, %rbx
     504: 72 39                        	jb	0x53f <rustium_http_range_content+0x53f>
     506: e9 c3 fc ff ff               	jmp	0x1ce <rustium_http_range_content+0x1ce>
     50b: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x512 <rustium_http_range_content+0x512>
     512: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x519 <rustium_http_range_content+0x519>
     519: ba ad 01 00 00               	movl	$0x1ad, %edx            # imm = 0x1AD
     51e: 45 31 c0                     	xorl	%r8d, %r8d
     521: e8 00 00 00 00               	callq	0x526 <rustium_http_range_content+0x526>
     526: 0f 0b                        	ud2
     528: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x52f <rustium_http_range_content+0x52f>
     52f: e8 00 00 00 00               	callq	0x534 <rustium_http_range_content+0x534>
     534: 0f 0b                        	ud2
     536: 48 39 cb                     	cmpq	%rcx, %rbx
     539: 0f 83 cb 00 00 00            	jae	0x60a <rustium_http_range_content+0x60a>
     53f: 48 83 7c 24 28 00            	cmpq	$0x0, 0x28(%rsp)
     545: 0f 95 c0                     	setne	%al
     548: 48 83 7c 24 38 00            	cmpq	$0x0, 0x38(%rsp)
     54e: 0f 99 c1                     	setns	%cl
     551: 84 c8                        	testb	%cl, %al
     553: 74 36                        	je	0x58b <rustium_http_range_content+0x58b>
     555: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
     55a: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
     55f: 48 01 c1                     	addq	%rax, %rcx
     562: 48 ff c1                     	incq	%rcx
     565: 48 83 7c 24 58 00            	cmpq	$0x0, 0x58(%rsp)
     56b: 0f 84 bd 00 00 00            	je	0x62e <rustium_http_range_content+0x62e>
     571: 48 8b 44 24 50               	movq	0x50(%rsp), %rax
     576: 48 03 44 24 28               	addq	0x28(%rsp), %rax
     57b: 48 89 44 24 50               	movq	%rax, 0x50(%rsp)
     580: 31 d2                        	xorl	%edx, %edx
     582: 31 c0                        	xorl	%eax, %eax
     584: 4c 8b 44 24 58               	movq	0x58(%rsp), %r8
     589: eb 25                        	jmp	0x5b0 <rustium_http_range_content+0x5b0>
     58b: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x592 <rustium_http_range_content+0x592>
     592: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x599 <rustium_http_range_content+0x599>
     599: ba 2f 02 00 00               	movl	$0x22f, %edx            # imm = 0x22F
     59e: 45 31 c0                     	xorl	%r8d, %r8d
     5a1: e8 00 00 00 00               	callq	0x5a6 <rustium_http_range_content+0x5a6>
     5a6: 0f 0b                        	ud2
     5a8: 48 ff c0                     	incq	%rax
     5ab: 49 39 c0                     	cmpq	%rax, %r8
     5ae: 74 77                        	je	0x627 <rustium_http_range_content+0x627>
     5b0: 44 0f b6 0c 01               	movzbl	(%rcx,%rax), %r9d
     5b5: 41 83 f9 09                  	cmpl	$0x9, %r9d
     5b9: 74 ed                        	je	0x5a8 <rustium_http_range_content+0x5a8>
     5bb: 41 83 f9 20                  	cmpl	$0x20, %r9d
     5bf: 74 e7                        	je	0x5a8 <rustium_http_range_content+0x5a8>
     5c1: 48 8b 54 24 70               	movq	0x70(%rsp), %rdx
     5c6: 4d 8d 0c 10                  	leaq	(%r8,%rdx), %r9
     5ca: 48 8b 54 24 28               	movq	0x28(%rsp), %rdx
     5cf: 48 01 f2                     	addq	%rsi, %rdx
     5d2: 49 01 d1                     	addq	%rdx, %r9
     5d5: 48 01 c1                     	addq	%rax, %rcx
     5d8: 31 d2                        	xorl	%edx, %edx
     5da: 45 31 d2                     	xorl	%r10d, %r10d
     5dd: eb 0f                        	jmp	0x5ee <rustium_http_range_content+0x5ee>
     5df: 4f 8d 1c 02                  	leaq	(%r10,%r8), %r11
     5e3: 49 ff cb                     	decq	%r11
     5e6: 49 ff ca                     	decq	%r10
     5e9: 49 39 c3                     	cmpq	%rax, %r11
     5ec: 74 42                        	je	0x630 <rustium_http_range_content+0x630>
     5ee: 47 0f b6 1c 11               	movzbl	(%r9,%r10), %r11d
     5f3: 41 83 fb 20                  	cmpl	$0x20, %r11d
     5f7: 74 e6                        	je	0x5df <rustium_http_range_content+0x5df>
     5f9: 41 83 fb 09                  	cmpl	$0x9, %r11d
     5fd: 74 e0                        	je	0x5df <rustium_http_range_content+0x5df>
     5ff: 4c 89 c2                     	movq	%r8, %rdx
     602: 48 29 c2                     	subq	%rax, %rdx
     605: 4c 01 d2                     	addq	%r10, %rdx
     608: eb 26                        	jmp	0x630 <rustium_http_range_content+0x630>
     60a: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x611 <rustium_http_range_content+0x611>
     611: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x618 <rustium_http_range_content+0x618>
     618: ba bb 01 00 00               	movl	$0x1bb, %edx            # imm = 0x1BB
     61d: 45 31 c0                     	xorl	%r8d, %r8d
     620: e8 00 00 00 00               	callq	0x625 <rustium_http_range_content+0x625>
     625: 0f 0b                        	ud2
     627: 48 8b 4c 24 50               	movq	0x50(%rsp), %rcx
     62c: eb 02                        	jmp	0x630 <rustium_http_range_content+0x630>
     62e: 31 d2                        	xorl	%edx, %edx
     630: e8 00 00 00 00               	callq	0x635 <rustium_http_range_content+0x635>
     635: 48 83 f8 01                  	cmpq	$0x1, %rax
     639: 0f 85 ec 01 00 00            	jne	0x82b <rustium_http_range_content+0x82b>
     63f: 48 8b 44 24 30               	movq	0x30(%rsp), %rax
     644: 4c 8d 34 03                  	leaq	(%rbx,%rax), %r14
     648: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
     64d: 48 89 10                     	movq	%rdx, (%rax)
     650: 4c 3b 74 24 38               	cmpq	0x38(%rsp), %r14
     655: 0f 87 10 02 00 00            	ja	0x86b <rustium_http_range_content+0x86b>
     65b: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
     660: 48 01 44 24 30               	addq	%rax, 0x30(%rsp)
     665: 48 85 db                     	testq	%rbx, %rbx
     668: 0f 84 97 00 00 00            	je	0x705 <rustium_http_range_content+0x705>
     66e: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
     673: 4a 8d 0c 30                  	leaq	(%rax,%r14), %rcx
     677: 31 d2                        	xorl	%edx, %edx
     679: 31 c0                        	xorl	%eax, %eax
     67b: 4c 8b 44 24 30               	movq	0x30(%rsp), %r8
     680: eb 08                        	jmp	0x68a <rustium_http_range_content+0x68a>
     682: 48 ff c0                     	incq	%rax
     685: 48 39 c3                     	cmpq	%rax, %rbx
     688: 74 74                        	je	0x6fe <rustium_http_range_content+0x6fe>
     68a: 45 0f b6 0c 00               	movzbl	(%r8,%rax), %r9d
     68f: 41 83 f9 09                  	cmpl	$0x9, %r9d
     693: 74 ed                        	je	0x682 <rustium_http_range_content+0x682>
     695: 41 83 f9 20                  	cmpl	$0x20, %r9d
     699: 74 e7                        	je	0x682 <rustium_http_range_content+0x682>
     69b: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
     6a0: 48 01 d9                     	addq	%rbx, %rcx
     6a3: 48 8b 54 24 68               	movq	0x68(%rsp), %rdx
     6a8: 48 01 ca                     	addq	%rcx, %rdx
     6ab: 48 8b 4c 24 28               	movq	0x28(%rsp), %rcx
     6b0: 48 01 d1                     	addq	%rdx, %rcx
     6b3: 48 ff c1                     	incq	%rcx
     6b6: 49 01 c0                     	addq	%rax, %r8
     6b9: 4c 89 44 24 30               	movq	%r8, 0x30(%rsp)
     6be: 31 d2                        	xorl	%edx, %edx
     6c0: 45 31 c0                     	xorl	%r8d, %r8d
     6c3: eb 0f                        	jmp	0x6d4 <rustium_http_range_content+0x6d4>
     6c5: 4d 8d 0c 18                  	leaq	(%r8,%rbx), %r9
     6c9: 49 ff c9                     	decq	%r9
     6cc: 49 ff c8                     	decq	%r8
     6cf: 49 39 c1                     	cmpq	%rax, %r9
     6d2: 74 33                        	je	0x707 <rustium_http_range_content+0x707>
     6d4: 46 0f b6 0c 01               	movzbl	(%rcx,%r8), %r9d
     6d9: 41 83 f9 20                  	cmpl	$0x20, %r9d
     6dd: 74 e6                        	je	0x6c5 <rustium_http_range_content+0x6c5>
     6df: 41 83 f9 09                  	cmpl	$0x9, %r9d
     6e3: 74 e0                        	je	0x6c5 <rustium_http_range_content+0x6c5>
     6e5: 48 89 da                     	movq	%rbx, %rdx
     6e8: 48 29 c2                     	subq	%rax, %rdx
     6eb: 4c 01 c2                     	addq	%r8, %rdx
     6ee: eb 17                        	jmp	0x707 <rustium_http_range_content+0x707>
     6f0: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x6f7 <rustium_http_range_content+0x6f7>
     6f7: e8 00 00 00 00               	callq	0x6fc <rustium_http_range_content+0x6fc>
     6fc: 0f 0b                        	ud2
     6fe: 48 89 4c 24 30               	movq	%rcx, 0x30(%rsp)
     703: eb 02                        	jmp	0x707 <rustium_http_range_content+0x707>
     705: 31 d2                        	xorl	%edx, %edx
     707: 48 8b 4c 24 30               	movq	0x30(%rsp), %rcx
     70c: e8 00 00 00 00               	callq	0x711 <rustium_http_range_content+0x711>
     711: 48 83 f8 01                  	cmpq	$0x1, %rax
     715: 48 8b 4c 24 48               	movq	0x48(%rsp), %rcx
     71a: 4c 8b 44 24 60               	movq	0x60(%rsp), %r8
     71f: 0f 85 e9 00 00 00            	jne	0x80e <rustium_http_range_content+0x80e>
     725: 49 89 10                     	movq	%rdx, (%r8)
     728: 48 3b 11                     	cmpq	(%rcx), %rdx
     72b: 0f 8c dd 00 00 00            	jl	0x80e <rustium_http_range_content+0x80e>
     731: 4c 89 44 24 60               	movq	%r8, 0x60(%rsp)
     736: 48 89 4c 24 48               	movq	%rcx, 0x48(%rsp)
     73b: 49 8d 4e 01                  	leaq	0x1(%r14), %rcx
     73f: 4c 39 f7                     	cmpq	%r14, %rdi
     742: 0f 86 3e 01 00 00            	jbe	0x886 <rustium_http_range_content+0x886>
     748: 48 39 cf                     	cmpq	%rcx, %rdi
     74b: 0f 84 89 00 00 00            	je	0x7da <rustium_http_range_content+0x7da>
     751: 48 8b 54 24 28               	movq	0x28(%rsp), %rdx
     756: 48 8d 0c 3a                  	leaq	(%rdx,%rdi), %rcx
     75a: 48 03 5c 24 58               	addq	0x58(%rsp), %rbx
     75f: 48 03 5c 24 70               	addq	0x70(%rsp), %rbx
     764: 48 89 d0                     	movq	%rdx, %rax
     767: 4c 8d 04 16                  	leaq	(%rsi,%rdx), %r8
     76b: 49 83 c0 03                  	addq	$0x3, %r8
     76f: 4d 8d 4c 24 fd               	leaq	-0x3(%r12), %r9
     774: 31 d2                        	xorl	%edx, %edx
     776: eb 08                        	jmp	0x780 <rustium_http_range_content+0x780>
     778: 48 ff c3                     	incq	%rbx
     77b: 49 39 d9                     	cmpq	%rbx, %r9
     77e: 74 67                        	je	0x7e7 <rustium_http_range_content+0x7e7>
     780: 45 0f b6 14 18               	movzbl	(%r8,%rbx), %r10d
     785: 41 83 fa 09                  	cmpl	$0x9, %r10d
     789: 74 ed                        	je	0x778 <rustium_http_range_content+0x778>
     78b: 41 83 fa 20                  	cmpl	$0x20, %r10d
     78f: 74 e7                        	je	0x778 <rustium_http_range_content+0x778>
     791: 48 89 c1                     	movq	%rax, %rcx
     794: 48 01 f8                     	addq	%rdi, %rax
     797: 48 ff c8                     	decq	%rax
     79a: 48 01 ce                     	addq	%rcx, %rsi
     79d: 48 8d 0c 33                  	leaq	(%rbx,%rsi), %rcx
     7a1: 48 83 c1 03                  	addq	$0x3, %rcx
     7a5: 31 d2                        	xorl	%edx, %edx
     7a7: 45 31 c0                     	xorl	%r8d, %r8d
     7aa: eb 10                        	jmp	0x7bc <rustium_http_range_content+0x7bc>
     7ac: 4f 8d 0c 04                  	leaq	(%r12,%r8), %r9
     7b0: 49 83 c1 fc                  	addq	$-0x4, %r9
     7b4: 49 ff c8                     	decq	%r8
     7b7: 49 39 d9                     	cmpq	%rbx, %r9
     7ba: 74 2b                        	je	0x7e7 <rustium_http_range_content+0x7e7>
     7bc: 46 0f b6 0c 00               	movzbl	(%rax,%r8), %r9d
     7c1: 41 83 f9 20                  	cmpl	$0x20, %r9d
     7c5: 74 e5                        	je	0x7ac <rustium_http_range_content+0x7ac>
     7c7: 41 83 f9 09                  	cmpl	$0x9, %r9d
     7cb: 74 df                        	je	0x7ac <rustium_http_range_content+0x7ac>
     7cd: 49 29 dc                     	subq	%rbx, %r12
     7d0: 4b 8d 14 04                  	leaq	(%r12,%r8), %rdx
     7d4: 48 83 c2 fd                  	addq	$-0x3, %rdx
     7d8: eb 0d                        	jmp	0x7e7 <rustium_http_range_content+0x7e7>
     7da: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
     7df: 48 01 c8                     	addq	%rcx, %rax
     7e2: 31 d2                        	xorl	%edx, %edx
     7e4: 48 89 c1                     	movq	%rax, %rcx
     7e7: e8 00 00 00 00               	callq	0x7ec <rustium_http_range_content+0x7ec>
     7ec: 48 83 f8 01                  	cmpq	$0x1, %rax
     7f0: 48 8b 4c 24 48               	movq	0x48(%rsp), %rcx
     7f5: 4c 8b 44 24 60               	movq	0x60(%rsp), %r8
     7fa: 75 12                        	jne	0x80e <rustium_http_range_content+0x80e>
     7fc: 48 8b 84 24 e0 00 00 00      	movq	0xe0(%rsp), %rax
     804: 48 89 10                     	movq	%rdx, (%rax)
     807: b0 01                        	movb	$0x1, %al
     809: 49 3b 10                     	cmpq	(%r8), %rdx
     80c: 7f 1f                        	jg	0x82d <rustium_http_range_content+0x82d>
     80e: 48 8b 84 24 e0 00 00 00      	movq	0xe0(%rsp), %rax
     816: 48 c7 00 ff ff ff ff         	movq	$-0x1, (%rax)
     81d: 49 c7 00 ff ff ff ff         	movq	$-0x1, (%r8)
     824: 48 c7 01 ff ff ff ff         	movq	$-0x1, (%rcx)
     82b: 31 c0                        	xorl	%eax, %eax
     82d: 48 83 c4 78                  	addq	$0x78, %rsp
     831: 5b                           	popq	%rbx
     832: 5d                           	popq	%rbp
     833: 5f                           	popq	%rdi
     834: 5e                           	popq	%rsi
     835: 41 5c                        	popq	%r12
     837: 41 5d                        	popq	%r13
     839: 41 5e                        	popq	%r14
     83b: 41 5f                        	popq	%r15
     83d: c3                           	retq
     83e: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x845 <rustium_http_range_content+0x845>
     845: 4c 89 d9                     	movq	%r11, %rcx
     848: 49 89 d0                     	movq	%rdx, %r8
     84b: 48 89 fa                     	movq	%rdi, %rdx
     84e: e8 00 00 00 00               	callq	0x853 <rustium_http_range_content+0x853>
     853: 0f 0b                        	ud2
     855: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x85c <rustium_http_range_content+0x85c>
     85c: 48 89 fa                     	movq	%rdi, %rdx
     85f: 4c 8b 44 24 38               	movq	0x38(%rsp), %r8
     864: e8 00 00 00 00               	callq	0x869 <rustium_http_range_content+0x869>
     869: 0f 0b                        	ud2
     86b: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x872 <rustium_http_range_content+0x872>
     872: 48 8b 4c 24 30               	movq	0x30(%rsp), %rcx
     877: 4c 89 f2                     	movq	%r14, %rdx
     87a: 4c 8b 44 24 38               	movq	0x38(%rsp), %r8
     87f: e8 00 00 00 00               	callq	0x884 <rustium_http_range_content+0x884>
     884: 0f 0b                        	ud2
     886: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x88d <rustium_http_range_content+0x88d>
     88d: eb cd                        	jmp	0x85c <rustium_http_range_content+0x85c>
     88f: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x896 <rustium_http_range_content+0x896>
     896: e8 00 00 00 00               	callq	0x89b <rustium_http_range_content+0x89b>
     89b: 0f 0b                        	ud2

Disassembly of section .text:

0000000000000000 <rustium_http_range_fetch>:
       0: 56                           	pushq	%rsi
       1: 48 83 ec 50                  	subq	$0x50, %rsp
       5: 48 85 d2                     	testq	%rdx, %rdx
       8: 74 31                        	je	0x3b <rustium_http_range_fetch+0x3b>
       a: 48 85 c9                     	testq	%rcx, %rcx
       d: 0f 95 c0                     	setne	%al
      10: 48 85 d2                     	testq	%rdx, %rdx
      13: 41 0f 99 c2                  	setns	%r10b
      17: 44 84 d0                     	testb	%r10b, %al
      1a: 0f 84 bf 01 00 00            	je	0x1df <rustium_http_range_fetch+0x1df>
      20: 48 83 fa 05                  	cmpq	$0x5, %rdx
      24: 72 15                        	jb	0x3b <rustium_http_range_fetch+0x3b>
      26: b8 62 79 74 65               	movl	$0x65747962, %eax       # imm = 0x65747962
      2b: 33 01                        	xorl	(%rcx), %eax
      2d: 44 0f b6 51 04               	movzbl	0x4(%rcx), %r10d
      32: 41 83 f2 73                  	xorl	$0x73, %r10d
      36: 41 09 c2                     	orl	%eax, %r10d
      39: 74 08                        	je	0x43 <rustium_http_range_fetch+0x43>
      3b: 31 c0                        	xorl	%eax, %eax
      3d: 48 83 c4 50                  	addq	$0x50, %rsp
      41: 5e                           	popq	%rsi
      42: c3                           	retq
      43: 48 83 fa 05                  	cmpq	$0x5, %rdx
      47: 41 0f 94 c2                  	sete	%r10b
      4b: 45 89 c3                     	movl	%r8d, %r11d
      4e: 41 80 f3 01                  	xorb	$0x1, %r11b
      52: b8 05 00 00 00               	movl	$0x5, %eax
      57: 45 08 d3                     	orb	%r10b, %r11b
      5a: 74 0c                        	je	0x68 <rustium_http_range_fetch+0x68>
      5c: eb 1b                        	jmp	0x79 <rustium_http_range_fetch+0x79>
      5e: 66 90                        	nop
      60: 48 ff c0                     	incq	%rax
      63: 48 39 c2                     	cmpq	%rax, %rdx
      66: 74 d3                        	je	0x3b <rustium_http_range_fetch+0x3b>
      68: 44 0f b6 14 01               	movzbl	(%rcx,%rax), %r10d
      6d: 41 83 fa 09                  	cmpl	$0x9, %r10d
      71: 74 ed                        	je	0x60 <rustium_http_range_fetch+0x60>
      73: 41 83 fa 20                  	cmpl	$0x20, %r10d
      77: 74 e7                        	je	0x60 <rustium_http_range_fetch+0x60>
      79: 48 39 d0                     	cmpq	%rdx, %rax
      7c: 73 bd                        	jae	0x3b <rustium_http_range_fetch+0x3b>
      7e: 80 3c 01 3d                  	cmpb	$0x3d, (%rcx,%rax)
      82: 75 b7                        	jne	0x3b <rustium_http_range_fetch+0x3b>
      84: 48 ff c0                     	incq	%rax
      87: 48 89 44 24 28               	movq	%rax, 0x28(%rsp)
      8c: 48 39 d0                     	cmpq	%rdx, %rax
      8f: 41 0f 92 c2                  	setb	%r10b
      93: 44 88 44 24 27               	movb	%r8b, 0x27(%rsp)
      98: 45 20 c2                     	andb	%r8b, %r10b
      9b: 41 80 fa 01                  	cmpb	$0x1, %r10b
      9f: 74 17                        	je	0xb8 <rustium_http_range_fetch+0xb8>
      a1: eb 30                        	jmp	0xd3 <rustium_http_range_fetch+0xd3>
      a3: 66 66 66 66 2e 0f 1f 84 00 00 00 00 00       	nopw	%cs:(%rax,%rax)
      b0: 48 ff c0                     	incq	%rax
      b3: 48 39 c2                     	cmpq	%rax, %rdx
      b6: 74 13                        	je	0xcb <rustium_http_range_fetch+0xcb>
      b8: 44 0f b6 04 01               	movzbl	(%rcx,%rax), %r8d
      bd: 41 83 f8 20                  	cmpl	$0x20, %r8d
      c1: 74 ed                        	je	0xb0 <rustium_http_range_fetch+0xb0>
      c3: 41 83 f8 09                  	cmpl	$0x9, %r8d
      c7: 74 e7                        	je	0xb0 <rustium_http_range_fetch+0xb0>
      c9: eb 03                        	jmp	0xce <rustium_http_range_fetch+0xce>
      cb: 48 89 d0                     	movq	%rdx, %rax
      ce: 48 89 44 24 28               	movq	%rax, 0x28(%rsp)
      d3: 4c 89 4c 24 48               	movq	%r9, 0x48(%rsp)
      d8: 4c 8d 44 24 28               	leaq	0x28(%rsp), %r8
      dd: 48 89 4c 24 30               	movq	%rcx, 0x30(%rsp)
      e2: 48 89 54 24 38               	movq	%rdx, 0x38(%rsp)
      e7: 48 8b 54 24 38               	movq	0x38(%rsp), %rdx
      ec: e8 00 00 00 00               	callq	0xf1 <rustium_http_range_fetch+0xf1>
      f1: 48 8b 74 24 38               	movq	0x38(%rsp), %rsi
      f6: 4c 8b 44 24 28               	movq	0x28(%rsp), %r8
      fb: 49 39 f0                     	cmpq	%rsi, %r8
      fe: 0f 92 c1                     	setb	%cl
     101: 44 0f b6 4c 24 27            	movzbl	0x27(%rsp), %r9d
     107: 44 20 c9                     	andb	%r9b, %cl
     10a: 80 f9 01                     	cmpb	$0x1, %cl
     10d: 75 30                        	jne	0x13f <rustium_http_range_fetch+0x13f>
     10f: 48 8b 4c 24 30               	movq	0x30(%rsp), %rcx
     114: eb 16                        	jmp	0x12c <rustium_http_range_fetch+0x12c>
     116: 66 2e 0f 1f 84 00 00 00 00 00	nopw	%cs:(%rax,%rax)
     120: 49 ff c0                     	incq	%r8
     123: 4c 39 c6                     	cmpq	%r8, %rsi
     126: 0f 84 0f ff ff ff            	je	0x3b <rustium_http_range_fetch+0x3b>
     12c: 46 0f b6 14 01               	movzbl	(%rcx,%r8), %r10d
     131: 41 83 fa 09                  	cmpl	$0x9, %r10d
     135: 74 e9                        	je	0x120 <rustium_http_range_fetch+0x120>
     137: 41 83 fa 20                  	cmpl	$0x20, %r10d
     13b: 74 e3                        	je	0x120 <rustium_http_range_fetch+0x120>
     13d: eb 05                        	jmp	0x144 <rustium_http_range_fetch+0x144>
     13f: 48 8b 4c 24 30               	movq	0x30(%rsp), %rcx
     144: 49 39 f0                     	cmpq	%rsi, %r8
     147: 0f 83 ee fe ff ff            	jae	0x3b <rustium_http_range_fetch+0x3b>
     14d: 42 80 3c 01 2d               	cmpb	$0x2d, (%rcx,%r8)
     152: 0f 85 e3 fe ff ff            	jne	0x3b <rustium_http_range_fetch+0x3b>
     158: 48 89 44 24 30               	movq	%rax, 0x30(%rsp)
     15d: 48 89 54 24 40               	movq	%rdx, 0x40(%rsp)
     162: 49 ff c0                     	incq	%r8
     165: 4c 89 44 24 28               	movq	%r8, 0x28(%rsp)
     16a: 49 39 f0                     	cmpq	%rsi, %r8
     16d: 0f 92 c0                     	setb	%al
     170: 41 20 c1                     	andb	%al, %r9b
     173: 41 80 f9 01                  	cmpb	$0x1, %r9b
     177: 74 0a                        	je	0x183 <rustium_http_range_fetch+0x183>
     179: eb 21                        	jmp	0x19c <rustium_http_range_fetch+0x19c>
     17b: 49 ff c0                     	incq	%r8
     17e: 4c 39 c6                     	cmpq	%r8, %rsi
     181: 74 11                        	je	0x194 <rustium_http_range_fetch+0x194>
     183: 42 0f b6 04 01               	movzbl	(%rcx,%r8), %eax
     188: 83 f8 20                     	cmpl	$0x20, %eax
     18b: 74 ee                        	je	0x17b <rustium_http_range_fetch+0x17b>
     18d: 83 f8 09                     	cmpl	$0x9, %eax
     190: 74 e9                        	je	0x17b <rustium_http_range_fetch+0x17b>
     192: eb 03                        	jmp	0x197 <rustium_http_range_fetch+0x197>
     194: 49 89 f0                     	movq	%rsi, %r8
     197: 4c 89 44 24 28               	movq	%r8, 0x28(%rsp)
     19c: 4c 8d 44 24 28               	leaq	0x28(%rsp), %r8
     1a1: 48 89 f2                     	movq	%rsi, %rdx
     1a4: e8 00 00 00 00               	callq	0x1a9 <rustium_http_range_fetch+0x1a9>
     1a9: 48 39 74 24 28               	cmpq	%rsi, 0x28(%rsp)
     1ae: 48 8b 4c 24 48               	movq	0x48(%rsp), %rcx
     1b3: 4c 8b 44 24 40               	movq	0x40(%rsp), %r8
     1b8: 4c 8b 4c 24 30               	movq	0x30(%rsp), %r9
     1bd: 0f 85 78 fe ff ff            	jne	0x3b <rustium_http_range_fetch+0x3b>
     1c3: 49 83 f9 01                  	cmpq	$0x1, %r9
     1c7: 75 33                        	jne	0x1fc <rustium_http_range_fetch+0x1fc>
     1c9: 45 31 c9                     	xorl	%r9d, %r9d
     1cc: 24 01                        	andb	$0x1, %al
     1ce: 3c 01                        	cmpb	$0x1, %al
     1d0: 4d 19 c9                     	sbbq	%r9, %r9
     1d3: 49 09 d1                     	orq	%rdx, %r9
     1d6: 48 c7 c2 ff ff ff ff         	movq	$-0x1, %rdx
     1dd: eb 33                        	jmp	0x212 <rustium_http_range_fetch+0x212>
     1df: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x1e6 <rustium_http_range_fetch+0x1e6>
     1e6: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x1ed <rustium_http_range_fetch+0x1ed>
     1ed: ba 2f 02 00 00               	movl	$0x22f, %edx            # imm = 0x22F
     1f2: 45 31 c0                     	xorl	%r8d, %r8d
     1f5: e8 00 00 00 00               	callq	0x1fa <rustium_http_range_fetch+0x1fa>
     1fa: 0f 0b                        	ud2
     1fc: a8 01                        	testb	$0x1, %al
     1fe: 0f 84 37 fe ff ff            	je	0x3b <rustium_http_range_fetch+0x3b>
     204: 49 c7 c0 ff ff ff ff         	movq	$-0x1, %r8
     20b: 49 c7 c1 ff ff ff ff         	movq	$-0x1, %r9
     212: f6 c1 07                     	testb	$0x7, %cl
     215: 0f 94 c0                     	sete	%al
     218: 48 85 c9                     	testq	%rcx, %rcx
     21b: 41 0f 95 c2                  	setne	%r10b
     21f: 41 84 c2                     	testb	%al, %r10b
     222: 74 13                        	je	0x237 <rustium_http_range_fetch+0x237>
     224: 4c 89 01                     	movq	%r8, (%rcx)
     227: 4c 89 49 08                  	movq	%r9, 0x8(%rcx)
     22b: 48 89 51 10                  	movq	%rdx, 0x10(%rcx)
     22f: b0 01                        	movb	$0x1, %al
     231: 48 83 c4 50                  	addq	$0x50, %rsp
     235: 5e                           	popq	%rsi
     236: c3                           	retq
     237: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x23e <rustium_http_range_fetch+0x23e>
     23e: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x245 <rustium_http_range_fetch+0x245>
     245: ba b7 01 00 00               	movl	$0x1b7, %edx            # imm = 0x1B7
     24a: 45 31 c0                     	xorl	%r8d, %r8d
     24d: e8 00 00 00 00               	callq	0x252 <rustium_http_range_fetch+0x252>
     252: 0f 0b                        	ud2

Disassembly of section .text:

0000000000000000 <rustium_http_range_list>:
       0: 41 57                        	pushq	%r15
       2: 41 56                        	pushq	%r14
       4: 41 55                        	pushq	%r13
       6: 41 54                        	pushq	%r12
       8: 56                           	pushq	%rsi
       9: 57                           	pushq	%rdi
       a: 55                           	pushq	%rbp
       b: 53                           	pushq	%rbx
       c: 48 81 ec 88 00 00 00         	subq	$0x88, %rsp
      13: 48 c7 44 24 38 00 00 00 00   	movq	$0x0, 0x38(%rsp)
      1c: 48 c7 44 24 40 08 00 00 00   	movq	$0x8, 0x40(%rsp)
      25: 48 c7 44 24 48 00 00 00 00   	movq	$0x0, 0x48(%rsp)
      2e: 48 85 d2                     	testq	%rdx, %rdx
      31: 0f 84 6e 01 00 00            	je	0x1a5 <rustium_http_range_list+0x1a5>
      37: 48 85 c9                     	testq	%rcx, %rcx
      3a: 0f 95 c0                     	setne	%al
      3d: 48 85 d2                     	testq	%rdx, %rdx
      40: 41 0f 99 c1                  	setns	%r9b
      44: 44 84 c8                     	testb	%r9b, %al
      47: 0f 84 f3 01 00 00            	je	0x240 <rustium_http_range_list+0x240>
      4d: 48 89 c8                     	movq	%rcx, %rax
      50: 48 01 d0                     	addq	%rdx, %rax
      53: 0f 82 63 05 00 00            	jb	0x5bc <rustium_http_range_list+0x5bc>
      59: 31 c0                        	xorl	%eax, %eax
      5b: 0f 1f 44 00 00               	nopl	(%rax,%rax)
      60: 80 3c 01 3d                  	cmpb	$0x3d, (%rcx,%rax)
      64: 74 0d                        	je	0x73 <rustium_http_range_list+0x73>
      66: 48 ff c0                     	incq	%rax
      69: 48 39 c2                     	cmpq	%rax, %rdx
      6c: 75 f2                        	jne	0x60 <rustium_http_range_list+0x60>
      6e: e9 32 01 00 00               	jmp	0x1a5 <rustium_http_range_list+0x1a5>
      73: 48 39 d0                     	cmpq	%rdx, %rax
      76: 0f 83 5d 05 00 00            	jae	0x5d9 <rustium_http_range_list+0x5d9>
      7c: 48 85 c0                     	testq	%rax, %rax
      7f: 0f 84 20 01 00 00            	je	0x1a5 <rustium_http_range_list+0x1a5>
      85: 49 89 c9                     	movq	%rcx, %r9
      88: 49 89 c2                     	movq	%rax, %r10
      8b: eb 0f                        	jmp	0x9c <rustium_http_range_list+0x9c>
      8d: 0f 1f 00                     	nopl	(%rax)
      90: 49 ff c1                     	incq	%r9
      93: 49 ff ca                     	decq	%r10
      96: 0f 84 09 01 00 00            	je	0x1a5 <rustium_http_range_list+0x1a5>
      9c: 45 0f b6 19                  	movzbl	(%r9), %r11d
      a0: 41 83 fb 09                  	cmpl	$0x9, %r11d
      a4: 74 ea                        	je	0x90 <rustium_http_range_list+0x90>
      a6: 41 83 fb 20                  	cmpl	$0x20, %r11d
      aa: 74 e4                        	je	0x90 <rustium_http_range_list+0x90>
      ac: 48 c7 c6 ff ff ff ff         	movq	$-0x1, %rsi
      b3: eb 1f                        	jmp	0xd4 <rustium_http_range_list+0xd4>
      b5: 66 66 2e 0f 1f 84 00 00 00 00 00     	nopw	%cs:(%rax,%rax)
      c0: 49 8d 3c 32                  	leaq	(%r10,%rsi), %rdi
      c4: 48 ff cf                     	decq	%rdi
      c7: 48 ff ce                     	decq	%rsi
      ca: 48 83 ff ff                  	cmpq	$-0x1, %rdi
      ce: 0f 84 d1 00 00 00            	je	0x1a5 <rustium_http_range_list+0x1a5>
      d4: 48 8d 3c 31                  	leaq	(%rcx,%rsi), %rdi
      d8: 0f b6 3c 38                  	movzbl	(%rax,%rdi), %edi
      dc: 83 ff 20                     	cmpl	$0x20, %edi
      df: 74 df                        	je	0xc0 <rustium_http_range_list+0xc0>
      e1: 83 ff 09                     	cmpl	$0x9, %edi
      e4: 74 da                        	je	0xc0 <rustium_http_range_list+0xc0>
      e6: 49 01 f2                     	addq	%rsi, %r10
      e9: 49 83 fa 04                  	cmpq	$0x4, %r10
      ed: 0f 85 b2 00 00 00            	jne	0x1a5 <rustium_http_range_list+0x1a5>
      f3: 45 8d 53 bf                  	leal	-0x41(%r11), %r10d
      f7: 41 80 fa 1a                  	cmpb	$0x1a, %r10b
      fb: 41 0f 92 c2                  	setb	%r10b
      ff: 41 c0 e2 05                  	shlb	$0x5, %r10b
     103: 45 08 da                     	orb	%r11b, %r10b
     106: 41 80 fa 62                  	cmpb	$0x62, %r10b
     10a: 0f 85 95 00 00 00            	jne	0x1a5 <rustium_http_range_list+0x1a5>
     110: 45 0f b6 51 01               	movzbl	0x1(%r9), %r10d
     115: 45 8d 5a bf                  	leal	-0x41(%r10), %r11d
     119: 41 80 fb 1a                  	cmpb	$0x1a, %r11b
     11d: 41 0f 92 c3                  	setb	%r11b
     121: 41 c0 e3 05                  	shlb	$0x5, %r11b
     125: 45 08 d3                     	orb	%r10b, %r11b
     128: 41 80 fb 79                  	cmpb	$0x79, %r11b
     12c: 75 77                        	jne	0x1a5 <rustium_http_range_list+0x1a5>
     12e: 45 0f b6 51 02               	movzbl	0x2(%r9), %r10d
     133: 45 8d 5a bf                  	leal	-0x41(%r10), %r11d
     137: 41 80 fb 1a                  	cmpb	$0x1a, %r11b
     13b: 41 0f 92 c3                  	setb	%r11b
     13f: 41 c0 e3 05                  	shlb	$0x5, %r11b
     143: 45 08 d3                     	orb	%r10b, %r11b
     146: 41 80 fb 74                  	cmpb	$0x74, %r11b
     14a: 75 59                        	jne	0x1a5 <rustium_http_range_list+0x1a5>
     14c: 45 0f b6 51 03               	movzbl	0x3(%r9), %r10d
     151: 45 8d 5a bf                  	leal	-0x41(%r10), %r11d
     155: 41 80 fb 1a                  	cmpb	$0x1a, %r11b
     159: 41 0f 92 c3                  	setb	%r11b
     15d: 41 c0 e3 05                  	shlb	$0x5, %r11b
     161: 45 08 d3                     	orb	%r10b, %r11b
     164: 41 80 fb 65                  	cmpb	$0x65, %r11b
     168: 75 3b                        	jne	0x1a5 <rustium_http_range_list+0x1a5>
     16a: 45 0f b6 49 04               	movzbl	0x4(%r9), %r9d
     16f: 45 8d 51 bf                  	leal	-0x41(%r9), %r10d
     173: 41 80 fa 1a                  	cmpb	$0x1a, %r10b
     177: 41 0f 92 c2                  	setb	%r10b
     17b: 41 c0 e2 05                  	shlb	$0x5, %r10b
     17f: 45 08 ca                     	orb	%r9b, %r10b
     182: 41 80 fa 73                  	cmpb	$0x73, %r10b
     186: 75 1d                        	jne	0x1a5 <rustium_http_range_list+0x1a5>
     188: 48 ff c0                     	incq	%rax
     18b: 40 b6 01                     	movb	$0x1, %sil
     18e: 48 39 d0                     	cmpq	%rdx, %rax
     191: 73 14                        	jae	0x1a7 <rustium_http_range_list+0x1a7>
     193: 4c 8d 59 ff                  	leaq	-0x1(%rcx), %r11
     197: 41 ba 08 00 00 00            	movl	$0x8, %r10d
     19d: 45 31 c9                     	xorl	%r9d, %r9d
     1a0: e9 32 01 00 00               	jmp	0x2d7 <rustium_http_range_list+0x2d7>
     1a5: 31 f6                        	xorl	%esi, %esi
     1a7: 48 8b 44 24 38               	movq	0x38(%rsp), %rax
     1ac: 48 85 c0                     	testq	%rax, %rax
     1af: 74 53                        	je	0x204 <rustium_http_range_list+0x204>
     1b1: 48 b9 ab aa aa aa aa aa aa 0a	movabsq	$0xaaaaaaaaaaaaaab, %rcx # imm = 0xAAAAAAAAAAAAAAB
     1bb: 48 39 c8                     	cmpq	%rcx, %rax
     1be: 0f 83 99 00 00 00            	jae	0x25d <rustium_http_range_list+0x25d>
     1c4: 48 b9 56 55 55 55 55 55 55 05	movabsq	$0x555555555555556, %rcx # imm = 0x555555555555556
     1ce: 48 39 c8                     	cmpq	%rcx, %rax
     1d1: 0f 83 a3 00 00 00            	jae	0x27a <rustium_http_range_list+0x27a>
     1d7: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
     1dc: 48 c1 e0 03                  	shlq	$0x3, %rax
     1e0: 48 8d 04 40                  	leaq	(%rax,%rax,2), %rax
     1e4: 48 c7 44 24 50 08 00 00 00   	movq	$0x8, 0x50(%rsp)
     1ed: 48 8d 94 24 80 00 00 00      	leaq	0x80(%rsp), %rdx
     1f5: 48 89 02                     	movq	%rax, (%rdx)
     1f8: 4c 8b 44 24 50               	movq	0x50(%rsp), %r8
     1fd: 4d 85 c0                     	testq	%r8, %r8
     200: 75 16                        	jne	0x218 <rustium_http_range_list+0x218>
     202: eb 26                        	jmp	0x22a <rustium_http_range_list+0x22a>
     204: 48 8d 54 24 50               	leaq	0x50(%rsp), %rdx
     209: 31 c0                        	xorl	%eax, %eax
     20b: 48 89 02                     	movq	%rax, (%rdx)
     20e: 4c 8b 44 24 50               	movq	0x50(%rsp), %r8
     213: 4d 85 c0                     	testq	%r8, %r8
     216: 74 12                        	je	0x22a <rustium_http_range_list+0x22a>
     218: 48 8b 94 24 80 00 00 00      	movq	0x80(%rsp), %rdx
     220: 48 85 d2                     	testq	%rdx, %rdx
     223: 74 05                        	je	0x22a <rustium_http_range_list+0x22a>
     225: e8 00 00 00 00               	callq	0x22a <rustium_http_range_list+0x22a>
     22a: 89 f0                        	movl	%esi, %eax
     22c: 48 81 c4 88 00 00 00         	addq	$0x88, %rsp
     233: 5b                           	popq	%rbx
     234: 5d                           	popq	%rbp
     235: 5f                           	popq	%rdi
     236: 5e                           	popq	%rsi
     237: 41 5c                        	popq	%r12
     239: 41 5d                        	popq	%r13
     23b: 41 5e                        	popq	%r14
     23d: 41 5f                        	popq	%r15
     23f: c3                           	retq
     240: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x247 <rustium_http_range_list+0x247>
     247: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x24e <rustium_http_range_list+0x24e>
     24e: ba 2f 02 00 00               	movl	$0x22f, %edx            # imm = 0x22F
     253: 45 31 c0                     	xorl	%r8d, %r8d
     256: e8 00 00 00 00               	callq	0x25b <rustium_http_range_list+0x25b>
     25b: 0f 0b                        	ud2
     25d: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x264 <rustium_http_range_list+0x264>
     264: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x26b <rustium_http_range_list+0x26b>
     26b: ba 75 01 00 00               	movl	$0x175, %edx            # imm = 0x175
     270: 45 31 c0                     	xorl	%r8d, %r8d
     273: e8 00 00 00 00               	callq	0x278 <rustium_http_range_list+0x278>
     278: 0f 0b                        	ud2
     27a: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x281 <rustium_http_range_list+0x281>
     281: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x288 <rustium_http_range_list+0x288>
     288: ba 33 02 00 00               	movl	$0x233, %edx            # imm = 0x233
     28d: 45 31 c0                     	xorl	%r8d, %r8d
     290: e8 00 00 00 00               	callq	0x295 <rustium_http_range_list+0x295>
     295: 0f 0b                        	ud2
     297: 4c 89 28                     	movq	%r13, (%rax)
     29a: 48 89 78 08                  	movq	%rdi, 0x8(%rax)
     29e: 48 89 58 10                  	movq	%rbx, 0x10(%rax)
     2a2: 49 ff c1                     	incq	%r9
     2a5: 4c 89 4c 24 48               	movq	%r9, 0x48(%rsp)
     2aa: 4c 8b 44 24 30               	movq	0x30(%rsp), %r8
     2af: 48 8b 54 24 58               	movq	0x58(%rsp), %rdx
     2b4: 48 8b 4c 24 60               	movq	0x60(%rsp), %rcx
     2b9: 4c 8b 5c 24 68               	movq	0x68(%rsp), %r11
     2be: 48 8b 74 24 70               	movq	0x70(%rsp), %rsi
     2c3: 48 8b 5c 24 78               	movq	0x78(%rsp), %rbx
     2c8: 48 01 f3                     	addq	%rsi, %rbx
     2cb: 48 89 d8                     	movq	%rbx, %rax
     2ce: 48 39 d3                     	cmpq	%rdx, %rbx
     2d1: 0f 83 2b 03 00 00            	jae	0x602 <rustium_http_range_list+0x602>
     2d7: 48 8d 78 01                  	leaq	0x1(%rax), %rdi
     2db: 48 39 fa                     	cmpq	%rdi, %rdx
     2de: 48 0f 47 fa                  	cmovaq	%rdx, %rdi
     2e2: 31 ed                        	xorl	%ebp, %ebp
     2e4: 48 89 c6                     	movq	%rax, %rsi
     2e7: 31 db                        	xorl	%ebx, %ebx
     2e9: eb 0a                        	jmp	0x2f5 <rustium_http_range_list+0x2f5>
     2eb: 31 db                        	xorl	%ebx, %ebx
     2ed: 48 ff c6                     	incq	%rsi
     2f0: 48 39 f7                     	cmpq	%rsi, %rdi
     2f3: 74 3f                        	je	0x334 <rustium_http_range_list+0x334>
     2f5: 44 0f b6 34 31               	movzbl	(%rcx,%rsi), %r14d
     2fa: 40 f6 c5 01                  	testb	$0x1, %bpl
     2fe: 74 1b                        	je	0x31b <rustium_http_range_list+0x31b>
     300: 40 b5 01                     	movb	$0x1, %bpl
     303: f6 c3 01                     	testb	$0x1, %bl
     306: 75 e3                        	jne	0x2eb <rustium_http_range_list+0x2eb>
     308: 41 83 fe 22                  	cmpl	$0x22, %r14d
     30c: 74 20                        	je	0x32e <rustium_http_range_list+0x32e>
     30e: 41 83 fe 5c                  	cmpl	$0x5c, %r14d
     312: 75 d7                        	jne	0x2eb <rustium_http_range_list+0x2eb>
     314: b3 01                        	movb	$0x1, %bl
     316: 40 b5 01                     	movb	$0x1, %bpl
     319: eb d2                        	jmp	0x2ed <rustium_http_range_list+0x2ed>
     31b: 41 83 fe 22                  	cmpl	$0x22, %r14d
     31f: 74 08                        	je	0x329 <rustium_http_range_list+0x329>
     321: 41 83 fe 2c                  	cmpl	$0x2c, %r14d
     325: 75 09                        	jne	0x330 <rustium_http_range_list+0x330>
     327: eb 12                        	jmp	0x33b <rustium_http_range_list+0x33b>
     329: 40 b5 01                     	movb	$0x1, %bpl
     32c: eb bf                        	jmp	0x2ed <rustium_http_range_list+0x2ed>
     32e: 31 db                        	xorl	%ebx, %ebx
     330: 31 ed                        	xorl	%ebp, %ebp
     332: eb b9                        	jmp	0x2ed <rustium_http_range_list+0x2ed>
     334: 48 89 fe                     	movq	%rdi, %rsi
     337: 31 db                        	xorl	%ebx, %ebx
     339: eb 05                        	jmp	0x340 <rustium_http_range_list+0x340>
     33b: bb 01 00 00 00               	movl	$0x1, %ebx
     340: 48 89 f7                     	movq	%rsi, %rdi
     343: 48 29 c7                     	subq	%rax, %rdi
     346: 74 80                        	je	0x2c8 <rustium_http_range_list+0x2c8>
     348: 4c 8d 34 01                  	leaq	(%rcx,%rax), %r14
     34c: 4d 8d 3c 03                  	leaq	(%r11,%rax), %r15
     350: 48 f7 d0                     	notq	%rax
     353: 48 01 f0                     	addq	%rsi, %rax
     356: eb 12                        	jmp	0x36a <rustium_http_range_list+0x36a>
     358: 49 ff c6                     	incq	%r14
     35b: 48 ff c8                     	decq	%rax
     35e: 49 ff c7                     	incq	%r15
     361: 48 ff cf                     	decq	%rdi
     364: 0f 84 5e ff ff ff            	je	0x2c8 <rustium_http_range_list+0x2c8>
     36a: 41 0f b6 2e                  	movzbl	(%r14), %ebp
     36e: 83 fd 09                     	cmpl	$0x9, %ebp
     371: 74 e5                        	je	0x358 <rustium_http_range_list+0x358>
     373: 83 fd 20                     	cmpl	$0x20, %ebp
     376: 74 e0                        	je	0x358 <rustium_http_range_list+0x358>
     378: 49 89 fc                     	movq	%rdi, %r12
     37b: 48 8d 3c 31                  	leaq	(%rcx,%rsi), %rdi
     37f: eb 10                        	jmp	0x391 <rustium_http_range_list+0x391>
     381: 48 ff cf                     	decq	%rdi
     384: 49 ff cc                     	decq	%r12
     387: 48 83 c0 ff                  	addq	$-0x1, %rax
     38b: 0f 83 37 ff ff ff            	jae	0x2c8 <rustium_http_range_list+0x2c8>
     391: 0f b6 6f ff                  	movzbl	-0x1(%rdi), %ebp
     395: 83 fd 20                     	cmpl	$0x20, %ebp
     398: 74 e7                        	je	0x381 <rustium_http_range_list+0x381>
     39a: 83 fd 09                     	cmpl	$0x9, %ebp
     39d: 74 e2                        	je	0x381 <rustium_http_range_list+0x381>
     39f: 48 89 c5                     	movq	%rax, %rbp
     3a2: 48 ff c5                     	incq	%rbp
     3a5: 0f 88 11 02 00 00            	js	0x5bc <rustium_http_range_list+0x5bc>
     3ab: 49 89 dd                     	movq	%rbx, %r13
     3ae: 4c 89 f3                     	movq	%r14, %rbx
     3b1: 48 f7 d3                     	notq	%rbx
     3b4: 48 39 dd                     	cmpq	%rbx, %rbp
     3b7: 0f 87 ff 01 00 00            	ja	0x5bc <rustium_http_range_list+0x5bc>
     3bd: 31 db                        	xorl	%ebx, %ebx
     3bf: 41 80 3c 1e 2d               	cmpb	$0x2d, (%r14,%rbx)
     3c4: 74 10                        	je	0x3d6 <rustium_http_range_list+0x3d6>
     3c6: 48 ff c3                     	incq	%rbx
     3c9: 49 ff c7                     	incq	%r15
     3cc: 49 ff cc                     	decq	%r12
     3cf: 75 ee                        	jne	0x3bf <rustium_http_range_list+0x3bf>
     3d1: e9 20 02 00 00               	jmp	0x5f6 <rustium_http_range_list+0x5f6>
     3d6: 4c 89 6c 24 78               	movq	%r13, 0x78(%rsp)
     3db: 48 89 74 24 70               	movq	%rsi, 0x70(%rsp)
     3e0: 4c 89 5c 24 68               	movq	%r11, 0x68(%rsp)
     3e5: 4c 89 4c 24 28               	movq	%r9, 0x28(%rsp)
     3ea: 48 89 4c 24 60               	movq	%rcx, 0x60(%rsp)
     3ef: 48 89 54 24 58               	movq	%rdx, 0x58(%rsp)
     3f4: 4c 89 44 24 30               	movq	%r8, 0x30(%rsp)
     3f9: 48 39 eb                     	cmpq	%rbp, %rbx
     3fc: 0f 83 d7 01 00 00            	jae	0x5d9 <rustium_http_range_list+0x5d9>
     402: 4c 89 d6                     	movq	%r10, %rsi
     405: 49 89 c4                     	movq	%rax, %r12
     408: 48 89 d8                     	movq	%rbx, %rax
     40b: 49 c7 c5 ff ff ff ff         	movq	$-0x1, %r13
     412: 48 85 db                     	testq	%rbx, %rbx
     415: 74 54                        	je	0x46b <rustium_http_range_list+0x46b>
     417: 45 31 c0                     	xorl	%r8d, %r8d
     41a: eb 08                        	jmp	0x424 <rustium_http_range_list+0x424>
     41c: 49 ff c0                     	incq	%r8
     41f: 4c 39 c0                     	cmpq	%r8, %rax
     422: 74 47                        	je	0x46b <rustium_http_range_list+0x46b>
     424: 43 0f b6 0c 06               	movzbl	(%r14,%r8), %ecx
     429: 83 f9 09                     	cmpl	$0x9, %ecx
     42c: 74 ee                        	je	0x41c <rustium_http_range_list+0x41c>
     42e: 83 f9 20                     	cmpl	$0x20, %ecx
     431: 74 e9                        	je	0x41c <rustium_http_range_list+0x41c>
     433: 4b 8d 0c 06                  	leaq	(%r14,%r8), %rcx
     437: 48 89 c2                     	movq	%rax, %rdx
     43a: 4c 29 c2                     	subq	%r8, %rdx
     43d: eb 0b                        	jmp	0x44a <rustium_http_range_list+0x44a>
     43f: 48 ff ca                     	decq	%rdx
     442: 49 ff cf                     	decq	%r15
     445: 48 85 d2                     	testq	%rdx, %rdx
     448: 74 1e                        	je	0x468 <rustium_http_range_list+0x468>
     44a: 41 0f b6 07                  	movzbl	(%r15), %eax
     44e: 83 f8 20                     	cmpl	$0x20, %eax
     451: 74 ec                        	je	0x43f <rustium_http_range_list+0x43f>
     453: 83 f8 09                     	cmpl	$0x9, %eax
     456: 74 e7                        	je	0x43f <rustium_http_range_list+0x43f>
     458: e8 00 00 00 00               	callq	0x45d <rustium_http_range_list+0x45d>
     45d: a8 01                        	testb	$0x1, %al
     45f: 0f 84 d8 01 00 00            	je	0x63d <rustium_http_range_list+0x63d>
     465: 49 89 d5                     	movq	%rdx, %r13
     468: 48 89 d8                     	movq	%rbx, %rax
     46b: 49 29 c4                     	subq	%rax, %r12
     46e: 75 77                        	jne	0x4e7 <rustium_http_range_list+0x4e7>
     470: 48 c7 c3 ff ff ff ff         	movq	$-0x1, %rbx
     477: 49 83 fd ff                  	cmpq	$-0x1, %r13
     47b: 0f 84 bc 01 00 00            	je	0x63d <rustium_http_range_list+0x63d>
     481: 48 c7 c7 ff ff ff ff         	movq	$-0x1, %rdi
     488: 48 85 db                     	testq	%rbx, %rbx
     48b: 7f 09                        	jg	0x496 <rustium_http_range_list+0x496>
     48d: 4d 85 ed                     	testq	%r13, %r13
     490: 0f 88 a7 01 00 00            	js	0x63d <rustium_http_range_list+0x63d>
     496: 4c 8b 4c 24 28               	movq	0x28(%rsp), %r9
     49b: 4c 3b 4c 24 38               	cmpq	0x38(%rsp), %r9
     4a0: 0f 84 c8 00 00 00            	je	0x56e <rustium_http_range_list+0x56e>
     4a6: 4c 89 c8                     	movq	%r9, %rax
     4a9: b9 18 00 00 00               	movl	$0x18, %ecx
     4ae: 48 f7 e1                     	mulq	%rcx
     4b1: 0f 82 9c 01 00 00            	jb	0x653 <rustium_http_range_list+0x653>
     4b7: 48 85 c0                     	testq	%rax, %rax
     4ba: 0f 88 93 01 00 00            	js	0x653 <rustium_http_range_list+0x653>
     4c0: 49 89 f2                     	movq	%rsi, %r10
     4c3: 48 89 f1                     	movq	%rsi, %rcx
     4c6: 48 f7 d1                     	notq	%rcx
     4c9: 48 39 c8                     	cmpq	%rcx, %rax
     4cc: 0f 87 81 01 00 00            	ja	0x653 <rustium_http_range_list+0x653>
     4d2: 4b 8d 04 49                  	leaq	(%r9,%r9,2), %rax
     4d6: 49 8d 04 c2                  	leaq	(%r10,%rax,8), %rax
     4da: a8 07                        	testb	$0x7, %al
     4dc: 0f 84 b5 fd ff ff            	je	0x297 <rustium_http_range_list+0x297>
     4e2: e9 a6 01 00 00               	jmp	0x68d <rustium_http_range_list+0x68d>
     4e7: 49 8d 0c 06                  	leaq	(%r14,%rax), %rcx
     4eb: 48 ff c1                     	incq	%rcx
     4ee: eb 0c                        	jmp	0x4fc <rustium_http_range_list+0x4fc>
     4f0: 48 ff c1                     	incq	%rcx
     4f3: 49 ff cc                     	decq	%r12
     4f6: 0f 84 74 ff ff ff            	je	0x470 <rustium_http_range_list+0x470>
     4fc: 0f b6 01                     	movzbl	(%rcx), %eax
     4ff: 83 f8 09                     	cmpl	$0x9, %eax
     502: 74 ec                        	je	0x4f0 <rustium_http_range_list+0x4f0>
     504: 83 f8 20                     	cmpl	$0x20, %eax
     507: 74 e7                        	je	0x4f0 <rustium_http_range_list+0x4f0>
     509: 31 c0                        	xorl	%eax, %eax
     50b: eb 0f                        	jmp	0x51c <rustium_http_range_list+0x51c>
     50d: 48 ff c8                     	decq	%rax
     510: 4d 89 e0                     	movq	%r12, %r8
     513: 49 01 c0                     	addq	%rax, %r8
     516: 0f 84 54 ff ff ff            	je	0x470 <rustium_http_range_list+0x470>
     51c: 44 0f b6 44 07 ff            	movzbl	-0x1(%rdi,%rax), %r8d
     522: 41 83 f8 20                  	cmpl	$0x20, %r8d
     526: 74 e5                        	je	0x50d <rustium_http_range_list+0x50d>
     528: 41 83 f8 09                  	cmpl	$0x9, %r8d
     52c: 74 df                        	je	0x50d <rustium_http_range_list+0x50d>
     52e: 49 01 c4                     	addq	%rax, %r12
     531: 4c 89 e2                     	movq	%r12, %rdx
     534: e8 00 00 00 00               	callq	0x539 <rustium_http_range_list+0x539>
     539: a8 01                        	testb	$0x1, %al
     53b: 0f 84 fc 00 00 00            	je	0x63d <rustium_http_range_list+0x63d>
     541: 48 89 d7                     	movq	%rdx, %rdi
     544: 49 83 fd ff                  	cmpq	$-0x1, %r13
     548: 74 4e                        	je	0x598 <rustium_http_range_list+0x598>
     54a: 4d 85 ed                     	testq	%r13, %r13
     54d: 0f 88 ea 00 00 00            	js	0x63d <rustium_http_range_list+0x63d>
     553: 48 c7 c3 ff ff ff ff         	movq	$-0x1, %rbx
     55a: 48 83 ff ff                  	cmpq	$-0x1, %rdi
     55e: 74 50                        	je	0x5b0 <rustium_http_range_list+0x5b0>
     560: 4c 39 ef                     	cmpq	%r13, %rdi
     563: 0f 8d 2d ff ff ff            	jge	0x496 <rustium_http_range_list+0x496>
     569: e9 cf 00 00 00               	jmp	0x63d <rustium_http_range_list+0x63d>
     56e: 48 8d 4c 24 38               	leaq	0x38(%rsp), %rcx
     573: e8 00 00 00 00               	callq	0x578 <rustium_http_range_list+0x578>
     578: 4c 8b 4c 24 28               	movq	0x28(%rsp), %r9
     57d: 48 8b 74 24 40               	movq	0x40(%rsp), %rsi
     582: 4c 89 c8                     	movq	%r9, %rax
     585: b9 18 00 00 00               	movl	$0x18, %ecx
     58a: 48 f7 e1                     	mulq	%rcx
     58d: 0f 83 24 ff ff ff            	jae	0x4b7 <rustium_http_range_list+0x4b7>
     593: e9 bb 00 00 00               	jmp	0x653 <rustium_http_range_list+0x653>
     598: 48 89 fb                     	movq	%rdi, %rbx
     59b: 48 c7 c7 ff ff ff ff         	movq	$-0x1, %rdi
     5a2: 48 85 db                     	testq	%rbx, %rbx
     5a5: 0f 8e e2 fe ff ff            	jle	0x48d <rustium_http_range_list+0x48d>
     5ab: e9 e6 fe ff ff               	jmp	0x496 <rustium_http_range_list+0x496>
     5b0: 48 c7 c7 ff ff ff ff         	movq	$-0x1, %rdi
     5b7: e9 da fe ff ff               	jmp	0x496 <rustium_http_range_list+0x496>
     5bc: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x5c3 <rustium_http_range_list+0x5c3>
     5c3: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x5ca <rustium_http_range_list+0x5ca>
     5ca: ba ad 01 00 00               	movl	$0x1ad, %edx            # imm = 0x1AD
     5cf: 45 31 c0                     	xorl	%r8d, %r8d
     5d2: e8 00 00 00 00               	callq	0x5d7 <rustium_http_range_list+0x5d7>
     5d7: 0f 0b                        	ud2
     5d9: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x5e0 <rustium_http_range_list+0x5e0>
     5e0: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x5e7 <rustium_http_range_list+0x5e7>
     5e7: ba bb 01 00 00               	movl	$0x1bb, %edx            # imm = 0x1BB
     5ec: 45 31 c0                     	xorl	%r8d, %r8d
     5ef: e8 00 00 00 00               	callq	0x5f4 <rustium_http_range_list+0x5f4>
     5f4: 0f 0b                        	ud2
     5f6: 31 f6                        	xorl	%esi, %esi
     5f8: 4d 85 c9                     	testq	%r9, %r9
     5fb: 75 11                        	jne	0x60e <rustium_http_range_list+0x60e>
     5fd: e9 a5 fb ff ff               	jmp	0x1a7 <rustium_http_range_list+0x1a7>
     602: 40 b6 01                     	movb	$0x1, %sil
     605: 4d 85 c9                     	testq	%r9, %r9
     608: 0f 84 99 fb ff ff            	je	0x1a7 <rustium_http_range_list+0x1a7>
     60e: 41 f6 c0 07                  	testb	$0x7, %r8b
     612: 0f 94 c0                     	sete	%al
     615: 4d 85 c0                     	testq	%r8, %r8
     618: 0f 95 c1                     	setne	%cl
     61b: 84 c1                        	testb	%al, %cl
     61d: 74 51                        	je	0x670 <rustium_http_range_list+0x670>
     61f: 48 8b 44 24 38               	movq	0x38(%rsp), %rax
     624: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
     629: 49 89 08                     	movq	%rcx, (%r8)
     62c: 4d 89 48 08                  	movq	%r9, 0x8(%r8)
     630: 49 89 40 10                  	movq	%rax, 0x10(%r8)
     634: 40 80 ce 02                  	orb	$0x2, %sil
     638: e9 ed fb ff ff               	jmp	0x22a <rustium_http_range_list+0x22a>
     63d: 31 f6                        	xorl	%esi, %esi
     63f: 4c 8b 44 24 30               	movq	0x30(%rsp), %r8
     644: 4c 8b 4c 24 28               	movq	0x28(%rsp), %r9
     649: 4d 85 c9                     	testq	%r9, %r9
     64c: 75 c0                        	jne	0x60e <rustium_http_range_list+0x60e>
     64e: e9 54 fb ff ff               	jmp	0x1a7 <rustium_http_range_list+0x1a7>
     653: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x65a <rustium_http_range_list+0x65a>
     65a: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x661 <rustium_http_range_list+0x661>
     661: ba ad 01 00 00               	movl	$0x1ad, %edx            # imm = 0x1AD
     666: 45 31 c0                     	xorl	%r8d, %r8d
     669: e8 00 00 00 00               	callq	0x66e <rustium_http_range_list+0x66e>
     66e: 0f 0b                        	ud2
     670: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x677 <rustium_http_range_list+0x677>
     677: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x67e <rustium_http_range_list+0x67e>
     67e: ba b7 01 00 00               	movl	$0x1b7, %edx            # imm = 0x1B7
     683: 45 31 c0                     	xorl	%r8d, %r8d
     686: e8 00 00 00 00               	callq	0x68b <rustium_http_range_list+0x68b>
     68b: 0f 0b                        	ud2
     68d: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x694 <rustium_http_range_list+0x694>
     694: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x69b <rustium_http_range_list+0x69b>
     69b: ba b7 01 00 00               	movl	$0x1b7, %edx            # imm = 0x1B7
     6a0: 45 31 c0                     	xorl	%r8d, %r8d
     6a3: e8 00 00 00 00               	callq	0x6a8 <rustium_http_range_list+0x6a8>
     6a8: 0f 0b                        	ud2

Disassembly of section .text:

0000000000000000 <rustium_http_range_list_drop>:
       0: 48 83 ec 38                  	subq	$0x38, %rsp
       4: 48 8b 41 10                  	movq	0x10(%rcx), %rax
       8: 48 39 41 08                  	cmpq	%rax, 0x8(%rcx)
       c: 0f 87 88 00 00 00            	ja	0x9a <rustium_http_range_list_drop+0x9a>
      12: 48 8b 09                     	movq	(%rcx), %rcx
      15: 48 85 c9                     	testq	%rcx, %rcx
      18: 0f 84 99 00 00 00            	je	0xb7 <rustium_http_range_list_drop+0xb7>
      1e: 48 85 c0                     	testq	%rax, %rax
      21: 74 4b                        	je	0x6e <rustium_http_range_list_drop+0x6e>
      23: 48 ba ab aa aa aa aa aa aa 0a	movabsq	$0xaaaaaaaaaaaaaab, %rdx # imm = 0xAAAAAAAAAAAAAAB
      2d: 48 39 d0                     	cmpq	%rdx, %rax
      30: 0f 83 9e 00 00 00            	jae	0xd4 <rustium_http_range_list_drop+0xd4>
      36: 48 ba 56 55 55 55 55 55 55 05	movabsq	$0x555555555555556, %rdx # imm = 0x555555555555556
      40: 48 39 d0                     	cmpq	%rdx, %rax
      43: 0f 83 a8 00 00 00            	jae	0xf1 <rustium_http_range_list_drop+0xf1>
      49: 48 c1 e0 03                  	shlq	$0x3, %rax
      4d: 48 8d 04 40                  	leaq	(%rax,%rax,2), %rax
      51: 48 c7 44 24 28 08 00 00 00   	movq	$0x8, 0x28(%rsp)
      5a: 48 8d 54 24 30               	leaq	0x30(%rsp), %rdx
      5f: 48 89 02                     	movq	%rax, (%rdx)
      62: 4c 8b 44 24 28               	movq	0x28(%rsp), %r8
      67: 4d 85 c0                     	testq	%r8, %r8
      6a: 75 16                        	jne	0x82 <rustium_http_range_list_drop+0x82>
      6c: eb 27                        	jmp	0x95 <rustium_http_range_list_drop+0x95>
      6e: 48 8d 54 24 28               	leaq	0x28(%rsp), %rdx
      73: 31 c0                        	xorl	%eax, %eax
      75: 48 89 02                     	movq	%rax, (%rdx)
      78: 4c 8b 44 24 28               	movq	0x28(%rsp), %r8
      7d: 4d 85 c0                     	testq	%r8, %r8
      80: 74 13                        	je	0x95 <rustium_http_range_list_drop+0x95>
      82: 48 8b 54 24 30               	movq	0x30(%rsp), %rdx
      87: 48 85 d2                     	testq	%rdx, %rdx
      8a: 74 09                        	je	0x95 <rustium_http_range_list_drop+0x95>
      8c: 48 83 c4 38                  	addq	$0x38, %rsp
      90: e9 00 00 00 00               	jmp	0x95 <rustium_http_range_list_drop+0x95>
      95: 48 83 c4 38                  	addq	$0x38, %rsp
      99: c3                           	retq
      9a: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xa1 <rustium_http_range_list_drop+0xa1>
      a1: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0xa8 <rustium_http_range_list_drop+0xa8>
      a8: ba 9b 01 00 00               	movl	$0x19b, %edx            # imm = 0x19B
      ad: 45 31 c0                     	xorl	%r8d, %r8d
      b0: e8 00 00 00 00               	callq	0xb5 <rustium_http_range_list_drop+0xb5>
      b5: 0f 0b                        	ud2
      b7: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xbe <rustium_http_range_list_drop+0xbe>
      be: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0xc5 <rustium_http_range_list_drop+0xc5>
      c5: ba a5 01 00 00               	movl	$0x1a5, %edx            # imm = 0x1A5
      ca: 45 31 c0                     	xorl	%r8d, %r8d
      cd: e8 00 00 00 00               	callq	0xd2 <rustium_http_range_list_drop+0xd2>
      d2: 0f 0b                        	ud2
      d4: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xdb <rustium_http_range_list_drop+0xdb>
      db: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0xe2 <rustium_http_range_list_drop+0xe2>
      e2: ba 75 01 00 00               	movl	$0x175, %edx            # imm = 0x175
      e7: 45 31 c0                     	xorl	%r8d, %r8d
      ea: e8 00 00 00 00               	callq	0xef <rustium_http_range_list_drop+0xef>
      ef: 0f 0b                        	ud2
      f1: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xf8 <rustium_http_range_list_drop+0xf8>
      f8: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0xff <rustium_http_range_list_drop+0xff>
      ff: ba 33 02 00 00               	movl	$0x233, %edx            # imm = 0x233
     104: 45 31 c0                     	xorl	%r8d, %r8d
     107: e8 00 00 00 00               	callq	0x10c <rustium_http_range_list_drop+0x10c>
     10c: 0f 0b                        	ud2
