
D:\RustiumBuild\src\out\Default\obj\net\libparse_number_native_parser_983ea169.rlib(lib.rmeta):	file format coff-x86-64

D:\RustiumBuild\src\out\Default\obj\net\libparse_number_native_parser_983ea169.rlib(lib.rmeta-link):	file format coff-x86-64

D:\RustiumBuild\src\out\Default\obj\net\libparse_number_native_parser_983ea169.rlib(libparse_number_native_parser_983ea169.parse_number_native_parser_983ea169.34af982c79aa5593-cgu.0.rcgu.o):	file format coff-x86-64

Disassembly of section .text:

0000000000000000 <rustium_parse_number_native_i32>:
       0: 56                           	pushq	%rsi
       1: 53                           	pushq	%rbx
       2: 48 83 ec 28                  	subq	$0x28, %rsp
       6: 4c 8b 54 24 60               	movq	0x60(%rsp), %r10
       b: 48 85 d2                     	testq	%rdx, %rdx
       e: 0f 84 84 00 00 00            	je	0x98 <rustium_parse_number_native_i32+0x98>
      14: 48 85 c9                     	testq	%rcx, %rcx
      17: 0f 95 c0                     	setne	%al
      1a: 48 85 d2                     	testq	%rdx, %rdx
      1d: 41 0f 99 c3                  	setns	%r11b
      21: 44 84 d8                     	testb	%r11b, %al
      24: 0f 84 ec 01 00 00            	je	0x216 <rustium_parse_number_native_i32+0x216>
      2a: 49 89 cb                     	movq	%rcx, %r11
      2d: 49 01 d3                     	addq	%rdx, %r11
      30: 0f 82 c3 01 00 00            	jb	0x1f9 <rustium_parse_number_native_i32+0x1f9>
      36: 0f b6 31                     	movzbl	(%rcx), %esi
      39: 8d 46 d0                     	leal	-0x30(%rsi), %eax
      3c: 3c 0a                        	cmpb	$0xa, %al
      3e: 73 5f                        	jae	0x9f <rustium_parse_number_native_i32+0x9f>
      40: 48 ff c1                     	incq	%rcx
      43: 89 f0                        	movl	%esi, %eax
      45: 41 83 e0 fe                  	andl	$-0x2, %r8d
      49: 41 83 f8 02                  	cmpl	$0x2, %r8d
      4d: 0f 94 c2                     	sete	%dl
      50: 3c 30                        	cmpb	$0x30, %al
      52: 41 0f 94 c0                  	sete	%r8b
      56: 44 84 c2                     	testb	%r8b, %dl
      59: 74 68                        	je	0xc3 <rustium_parse_number_native_i32+0xc3>
      5b: 40 80 fe 2d                  	cmpb	$0x2d, %sil
      5f: 0f 94 c0                     	sete	%al
      62: 4c 39 d9                     	cmpq	%r11, %rcx
      65: 0f 95 c1                     	setne	%cl
      68: 31 d2                        	xorl	%edx, %edx
      6a: 08 c1                        	orb	%al, %cl
      6c: b8 02 00 00 00               	movl	$0x2, %eax
      71: 0f 85 78 00 00 00            	jne	0xef <rustium_parse_number_native_i32+0xef>
      77: 41 f6 c1 03                  	testb	$0x3, %r9b
      7b: 0f 94 c0                     	sete	%al
      7e: 4d 85 c9                     	testq	%r9, %r9
      81: 0f 95 c1                     	setne	%cl
      84: 84 c1                        	testb	%al, %cl
      86: 0f 84 c4 01 00 00            	je	0x250 <rustium_parse_number_native_i32+0x250>
      8c: 41 89 11                     	movl	%edx, (%r9)
      8f: b0 01                        	movb	$0x1, %al
      91: 48 83 c4 28                  	addq	$0x28, %rsp
      95: 5b                           	popq	%rbx
      96: 5e                           	popq	%rsi
      97: c3                           	retq
      98: b8 02 00 00 00               	movl	$0x2, %eax
      9d: eb 50                        	jmp	0xef <rustium_parse_number_native_i32+0xef>
      9f: b8 02 00 00 00               	movl	$0x2, %eax
      a4: 40 80 fe 2d                  	cmpb	$0x2d, %sil
      a8: 75 45                        	jne	0xef <rustium_parse_number_native_i32+0xef>
      aa: 41 f7 c0 fd ff ff ff         	testl	$0xfffffffd, %r8d       # imm = 0xFFFFFFFD
      b1: 74 3c                        	je	0xef <rustium_parse_number_native_i32+0xef>
      b3: 48 83 fa 01                  	cmpq	$0x1, %rdx
      b7: 74 36                        	je	0xef <rustium_parse_number_native_i32+0xef>
      b9: 0f b6 41 01                  	movzbl	0x1(%rcx), %eax
      bd: 48 83 c1 02                  	addq	$0x2, %rcx
      c1: eb 82                        	jmp	0x45 <rustium_parse_number_native_i32+0x45>
      c3: 3c 30                        	cmpb	$0x30, %al
      c5: 75 18                        	jne	0xdf <rustium_parse_number_native_i32+0xdf>
      c7: 31 d2                        	xorl	%edx, %edx
      c9: 0f 1f 80 00 00 00 00         	nopl	(%rax)
      d0: 4c 39 d9                     	cmpq	%r11, %rcx
      d3: 74 a2                        	je	0x77 <rustium_parse_number_native_i32+0x77>
      d5: 0f b6 01                     	movzbl	(%rcx), %eax
      d8: 48 ff c1                     	incq	%rcx
      db: 3c 30                        	cmpb	$0x30, %al
      dd: 74 f1                        	je	0xd0 <rustium_parse_number_native_i32+0xd0>
      df: 04 d0                        	addb	$-0x30, %al
      e1: 41 89 c0                     	movl	%eax, %r8d
      e4: b8 02 00 00 00               	movl	$0x2, %eax
      e9: 41 80 f8 09                  	cmpb	$0x9, %r8b
      ed: 76 1b                        	jbe	0x10a <rustium_parse_number_native_i32+0x10a>
      ef: 4d 85 d2                     	testq	%r10, %r10
      f2: 74 0d                        	je	0x101 <rustium_parse_number_native_i32+0x101>
      f4: 41 f6 c2 03                  	testb	$0x3, %r10b
      f8: 0f 85 35 01 00 00            	jne	0x233 <rustium_parse_number_native_i32+0x233>
      fe: 41 89 02                     	movl	%eax, (%r10)
     101: 31 c0                        	xorl	%eax, %eax
     103: 48 83 c4 28                  	addq	$0x28, %rsp
     107: 5b                           	popq	%rbx
     108: 5e                           	popq	%rsi
     109: c3                           	retq
     10a: 41 0f b6 d0                  	movzbl	%r8b, %edx
     10e: 40 80 fe 2d                  	cmpb	$0x2d, %sil
     112: 75 6a                        	jne	0x17e <rustium_parse_number_native_i32+0x17e>
     114: 4c 39 d9                     	cmpq	%r11, %rcx
     117: 75 18                        	jne	0x131 <rustium_parse_number_native_i32+0x131>
     119: 48 f7 da                     	negq	%rdx
     11c: e9 56 ff ff ff               	jmp	0x77 <rustium_parse_number_native_i32+0x77>
     121: 48 8d 0c 92                  	leaq	(%rdx,%rdx,4), %rcx
     125: 48 8d 14 4e                  	leaq	(%rsi,%rcx,2), %rdx
     129: 4c 89 c1                     	movq	%r8, %rcx
     12c: 4d 39 d8                     	cmpq	%r11, %r8
     12f: 74 e8                        	je	0x119 <rustium_parse_number_native_i32+0x119>
     131: 0f b6 19                     	movzbl	(%rcx), %ebx
     134: 80 c3 d0                     	addb	$-0x30, %bl
     137: 80 fb 09                     	cmpb	$0x9, %bl
     13a: 77 b3                        	ja	0xef <rustium_parse_number_native_i32+0xef>
     13c: 4c 8d 41 01                  	leaq	0x1(%rcx), %r8
     140: 48 81 fa cc cc cc 0c         	cmpq	$0xccccccc, %rdx        # imm = 0xCCCCCCC
     147: 77 0b                        	ja	0x154 <rustium_parse_number_native_i32+0x154>
     149: 0f b6 f3                     	movzbl	%bl, %esi
     14c: 75 d3                        	jne	0x121 <rustium_parse_number_native_i32+0x121>
     14e: 40 80 fe 09                  	cmpb	$0x9, %sil
     152: 75 cd                        	jne	0x121 <rustium_parse_number_native_i32+0x121>
     154: 4d 85 d2                     	testq	%r10, %r10
     157: 74 a8                        	je	0x101 <rustium_parse_number_native_i32+0x101>
     159: 48 ff c1                     	incq	%rcx
     15c: 4c 39 d9                     	cmpq	%r11, %rcx
     15f: 74 13                        	je	0x174 <rustium_parse_number_native_i32+0x174>
     161: 0f b6 11                     	movzbl	(%rcx), %edx
     164: 80 c2 c6                     	addb	$-0x3a, %dl
     167: 80 fa f6                     	cmpb	$-0xa, %dl
     16a: 72 83                        	jb	0xef <rustium_parse_number_native_i32+0xef>
     16c: 48 ff c1                     	incq	%rcx
     16f: 4c 39 d9                     	cmpq	%r11, %rcx
     172: 75 ed                        	jne	0x161 <rustium_parse_number_native_i32+0x161>
     174: b8 01 00 00 00               	movl	$0x1, %eax
     179: e9 71 ff ff ff               	jmp	0xef <rustium_parse_number_native_i32+0xef>
     17e: 4c 39 d9                     	cmpq	%r11, %rcx
     181: 75 19                        	jne	0x19c <rustium_parse_number_native_i32+0x19c>
     183: e9 ef fe ff ff               	jmp	0x77 <rustium_parse_number_native_i32+0x77>
     188: 48 8d 0c 92                  	leaq	(%rdx,%rdx,4), %rcx
     18c: 48 8d 14 4e                  	leaq	(%rsi,%rcx,2), %rdx
     190: 4c 89 c1                     	movq	%r8, %rcx
     193: 4d 39 d8                     	cmpq	%r11, %r8
     196: 0f 84 db fe ff ff            	je	0x77 <rustium_parse_number_native_i32+0x77>
     19c: 0f b6 19                     	movzbl	(%rcx), %ebx
     19f: 80 c3 d0                     	addb	$-0x30, %bl
     1a2: 80 fb 09                     	cmpb	$0x9, %bl
     1a5: 0f 87 44 ff ff ff            	ja	0xef <rustium_parse_number_native_i32+0xef>
     1ab: 4c 8d 41 01                  	leaq	0x1(%rcx), %r8
     1af: 48 81 fa cc cc cc 0c         	cmpq	$0xccccccc, %rdx        # imm = 0xCCCCCCC
     1b6: 77 0b                        	ja	0x1c3 <rustium_parse_number_native_i32+0x1c3>
     1b8: 0f b6 f3                     	movzbl	%bl, %esi
     1bb: 75 cb                        	jne	0x188 <rustium_parse_number_native_i32+0x188>
     1bd: 40 80 fe 07                  	cmpb	$0x7, %sil
     1c1: 76 c5                        	jbe	0x188 <rustium_parse_number_native_i32+0x188>
     1c3: 31 c0                        	xorl	%eax, %eax
     1c5: 4d 85 d2                     	testq	%r10, %r10
     1c8: 0f 84 c3 fe ff ff            	je	0x91 <rustium_parse_number_native_i32+0x91>
     1ce: 48 ff c1                     	incq	%rcx
     1d1: 4c 39 d9                     	cmpq	%r11, %rcx
     1d4: 74 1c                        	je	0x1f2 <rustium_parse_number_native_i32+0x1f2>
     1d6: b8 02 00 00 00               	movl	$0x2, %eax
     1db: 0f b6 11                     	movzbl	(%rcx), %edx
     1de: 80 c2 c6                     	addb	$-0x3a, %dl
     1e1: 80 fa f6                     	cmpb	$-0xa, %dl
     1e4: 0f 82 05 ff ff ff            	jb	0xef <rustium_parse_number_native_i32+0xef>
     1ea: 48 ff c1                     	incq	%rcx
     1ed: 4c 39 d9                     	cmpq	%r11, %rcx
     1f0: 75 e9                        	jne	0x1db <rustium_parse_number_native_i32+0x1db>
     1f2: 31 c0                        	xorl	%eax, %eax
     1f4: e9 f6 fe ff ff               	jmp	0xef <rustium_parse_number_native_i32+0xef>
     1f9: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x200 <rustium_parse_number_native_i32+0x200>
     200: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x207 <rustium_parse_number_native_i32+0x207>
     207: ba ad 01 00 00               	movl	$0x1ad, %edx            # imm = 0x1AD
     20c: 45 31 c0                     	xorl	%r8d, %r8d
     20f: e8 00 00 00 00               	callq	0x214 <rustium_parse_number_native_i32+0x214>
     214: 0f 0b                        	ud2
     216: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x21d <rustium_parse_number_native_i32+0x21d>
     21d: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x224 <rustium_parse_number_native_i32+0x224>
     224: ba 2f 02 00 00               	movl	$0x22f, %edx            # imm = 0x22F
     229: 45 31 c0                     	xorl	%r8d, %r8d
     22c: e8 00 00 00 00               	callq	0x231 <rustium_parse_number_native_i32+0x231>
     231: 0f 0b                        	ud2
     233: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x23a <rustium_parse_number_native_i32+0x23a>
     23a: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x241 <rustium_parse_number_native_i32+0x241>
     241: ba b7 01 00 00               	movl	$0x1b7, %edx            # imm = 0x1B7
     246: 45 31 c0                     	xorl	%r8d, %r8d
     249: e8 00 00 00 00               	callq	0x24e <rustium_parse_number_native_i32+0x24e>
     24e: 0f 0b                        	ud2
     250: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x257 <rustium_parse_number_native_i32+0x257>
     257: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x25e <rustium_parse_number_native_i32+0x25e>
     25e: ba b7 01 00 00               	movl	$0x1b7, %edx            # imm = 0x1B7
     263: 45 31 c0                     	xorl	%r8d, %r8d
     266: e8 00 00 00 00               	callq	0x26b <rustium_parse_number_native_i32+0x26b>
     26b: 0f 0b                        	ud2

Disassembly of section .text:

0000000000000000 <rustium_parse_number_native_i64>:
       0: 56                           	pushq	%rsi
       1: 57                           	pushq	%rdi
       2: 53                           	pushq	%rbx
       3: 48 83 ec 20                  	subq	$0x20, %rsp
       7: 4c 8b 54 24 60               	movq	0x60(%rsp), %r10
       c: 48 85 d2                     	testq	%rdx, %rdx
       f: 74 7b                        	je	0x8c <rustium_parse_number_native_i64+0x8c>
      11: 48 85 c9                     	testq	%rcx, %rcx
      14: 0f 95 c0                     	setne	%al
      17: 48 85 d2                     	testq	%rdx, %rdx
      1a: 41 0f 99 c3                  	setns	%r11b
      1e: 44 84 d8                     	testb	%r11b, %al
      21: 0f 84 fb 01 00 00            	je	0x222 <rustium_parse_number_native_i64+0x222>
      27: 49 89 cb                     	movq	%rcx, %r11
      2a: 49 01 d3                     	addq	%rdx, %r11
      2d: 0f 82 d2 01 00 00            	jb	0x205 <rustium_parse_number_native_i64+0x205>
      33: 0f b6 31                     	movzbl	(%rcx), %esi
      36: 8d 46 d0                     	leal	-0x30(%rsi), %eax
      39: 3c 0a                        	cmpb	$0xa, %al
      3b: 73 56                        	jae	0x93 <rustium_parse_number_native_i64+0x93>
      3d: 48 ff c1                     	incq	%rcx
      40: 89 f0                        	movl	%esi, %eax
      42: 41 83 e0 fe                  	andl	$-0x2, %r8d
      46: 41 83 f8 02                  	cmpl	$0x2, %r8d
      4a: 0f 94 c2                     	sete	%dl
      4d: 3c 30                        	cmpb	$0x30, %al
      4f: 41 0f 94 c0                  	sete	%r8b
      53: 44 84 c2                     	testb	%r8b, %dl
      56: 74 5f                        	je	0xb7 <rustium_parse_number_native_i64+0xb7>
      58: 40 80 fe 2d                  	cmpb	$0x2d, %sil
      5c: 0f 94 c0                     	sete	%al
      5f: 4c 39 d9                     	cmpq	%r11, %rcx
      62: 0f 95 c1                     	setne	%cl
      65: 08 c1                        	orb	%al, %cl
      67: b8 02 00 00 00               	movl	$0x2, %eax
      6c: 75 71                        	jne	0xdf <rustium_parse_number_native_i64+0xdf>
      6e: 31 d2                        	xorl	%edx, %edx
      70: 41 f6 c1 07                  	testb	$0x7, %r9b
      74: 0f 94 c0                     	sete	%al
      77: 4d 85 c9                     	testq	%r9, %r9
      7a: 0f 95 c1                     	setne	%cl
      7d: 84 c1                        	testb	%al, %cl
      7f: 0f 84 d7 01 00 00            	je	0x25c <rustium_parse_number_native_i64+0x25c>
      85: 49 89 11                     	movq	%rdx, (%r9)
      88: b0 01                        	movb	$0x1, %al
      8a: eb 67                        	jmp	0xf3 <rustium_parse_number_native_i64+0xf3>
      8c: b8 02 00 00 00               	movl	$0x2, %eax
      91: eb 4c                        	jmp	0xdf <rustium_parse_number_native_i64+0xdf>
      93: b8 02 00 00 00               	movl	$0x2, %eax
      98: 40 80 fe 2d                  	cmpb	$0x2d, %sil
      9c: 75 41                        	jne	0xdf <rustium_parse_number_native_i64+0xdf>
      9e: 41 f7 c0 fd ff ff ff         	testl	$0xfffffffd, %r8d       # imm = 0xFFFFFFFD
      a5: 74 38                        	je	0xdf <rustium_parse_number_native_i64+0xdf>
      a7: 48 83 fa 01                  	cmpq	$0x1, %rdx
      ab: 74 32                        	je	0xdf <rustium_parse_number_native_i64+0xdf>
      ad: 0f b6 41 01                  	movzbl	0x1(%rcx), %eax
      b1: 48 83 c1 02                  	addq	$0x2, %rcx
      b5: eb 8b                        	jmp	0x42 <rustium_parse_number_native_i64+0x42>
      b7: 3c 30                        	cmpb	$0x30, %al
      b9: 75 14                        	jne	0xcf <rustium_parse_number_native_i64+0xcf>
      bb: 31 d2                        	xorl	%edx, %edx
      bd: 0f 1f 00                     	nopl	(%rax)
      c0: 4c 39 d9                     	cmpq	%r11, %rcx
      c3: 74 ab                        	je	0x70 <rustium_parse_number_native_i64+0x70>
      c5: 0f b6 01                     	movzbl	(%rcx), %eax
      c8: 48 ff c1                     	incq	%rcx
      cb: 3c 30                        	cmpb	$0x30, %al
      cd: 74 f1                        	je	0xc0 <rustium_parse_number_native_i64+0xc0>
      cf: 04 d0                        	addb	$-0x30, %al
      d1: 41 89 c0                     	movl	%eax, %r8d
      d4: b8 02 00 00 00               	movl	$0x2, %eax
      d9: 41 80 f8 09                  	cmpb	$0x9, %r8b
      dd: 76 1c                        	jbe	0xfb <rustium_parse_number_native_i64+0xfb>
      df: 4d 85 d2                     	testq	%r10, %r10
      e2: 74 0d                        	je	0xf1 <rustium_parse_number_native_i64+0xf1>
      e4: 41 f6 c2 03                  	testb	$0x3, %r10b
      e8: 0f 85 51 01 00 00            	jne	0x23f <rustium_parse_number_native_i64+0x23f>
      ee: 41 89 02                     	movl	%eax, (%r10)
      f1: 31 c0                        	xorl	%eax, %eax
      f3: 48 83 c4 20                  	addq	$0x20, %rsp
      f7: 5b                           	popq	%rbx
      f8: 5f                           	popq	%rdi
      f9: 5e                           	popq	%rsi
      fa: c3                           	retq
      fb: 41 0f b6 d0                  	movzbl	%r8b, %edx
      ff: 40 80 fe 2d                  	cmpb	$0x2d, %sil
     103: 75 76                        	jne	0x17b <rustium_parse_number_native_i64+0x17b>
     105: 4c 39 d9                     	cmpq	%r11, %rcx
     108: 0f 84 ef 00 00 00            	je	0x1fd <rustium_parse_number_native_i64+0x1fd>
     10e: 49 b8 cc cc cc cc cc cc cc 0c	movabsq	$0xccccccccccccccc, %r8 # imm = 0xCCCCCCCCCCCCCCC
     118: eb 14                        	jmp	0x12e <rustium_parse_number_native_i64+0x12e>
     11a: 48 8d 0c 92                  	leaq	(%rdx,%rdx,4), %rcx
     11e: 48 8d 14 4f                  	leaq	(%rdi,%rcx,2), %rdx
     122: 48 89 f1                     	movq	%rsi, %rcx
     125: 4c 39 de                     	cmpq	%r11, %rsi
     128: 0f 84 cf 00 00 00            	je	0x1fd <rustium_parse_number_native_i64+0x1fd>
     12e: 0f b6 19                     	movzbl	(%rcx), %ebx
     131: 80 c3 d0                     	addb	$-0x30, %bl
     134: 80 fb 09                     	cmpb	$0x9, %bl
     137: 77 a6                        	ja	0xdf <rustium_parse_number_native_i64+0xdf>
     139: 48 8d 71 01                  	leaq	0x1(%rcx), %rsi
     13d: 4c 39 c2                     	cmpq	%r8, %rdx
     140: 77 0b                        	ja	0x14d <rustium_parse_number_native_i64+0x14d>
     142: 0f b6 fb                     	movzbl	%bl, %edi
     145: 75 d3                        	jne	0x11a <rustium_parse_number_native_i64+0x11a>
     147: 40 80 ff 09                  	cmpb	$0x9, %dil
     14b: 75 cd                        	jne	0x11a <rustium_parse_number_native_i64+0x11a>
     14d: 4d 85 d2                     	testq	%r10, %r10
     150: 74 9f                        	je	0xf1 <rustium_parse_number_native_i64+0xf1>
     152: 48 ff c1                     	incq	%rcx
     155: 4c 39 d9                     	cmpq	%r11, %rcx
     158: 74 17                        	je	0x171 <rustium_parse_number_native_i64+0x171>
     15a: 0f b6 11                     	movzbl	(%rcx), %edx
     15d: 80 c2 c6                     	addb	$-0x3a, %dl
     160: 80 fa f6                     	cmpb	$-0xa, %dl
     163: 0f 82 76 ff ff ff            	jb	0xdf <rustium_parse_number_native_i64+0xdf>
     169: 48 ff c1                     	incq	%rcx
     16c: 4c 39 d9                     	cmpq	%r11, %rcx
     16f: 75 e9                        	jne	0x15a <rustium_parse_number_native_i64+0x15a>
     171: b8 01 00 00 00               	movl	$0x1, %eax
     176: e9 64 ff ff ff               	jmp	0xdf <rustium_parse_number_native_i64+0xdf>
     17b: 4c 39 d9                     	cmpq	%r11, %rcx
     17e: 0f 84 ec fe ff ff            	je	0x70 <rustium_parse_number_native_i64+0x70>
     184: 49 b8 cc cc cc cc cc cc cc 0c	movabsq	$0xccccccccccccccc, %r8 # imm = 0xCCCCCCCCCCCCCCC
     18e: eb 14                        	jmp	0x1a4 <rustium_parse_number_native_i64+0x1a4>
     190: 48 8d 0c 92                  	leaq	(%rdx,%rdx,4), %rcx
     194: 48 8d 14 4f                  	leaq	(%rdi,%rcx,2), %rdx
     198: 48 89 f1                     	movq	%rsi, %rcx
     19b: 4c 39 de                     	cmpq	%r11, %rsi
     19e: 0f 84 cc fe ff ff            	je	0x70 <rustium_parse_number_native_i64+0x70>
     1a4: 0f b6 19                     	movzbl	(%rcx), %ebx
     1a7: 80 c3 d0                     	addb	$-0x30, %bl
     1aa: 80 fb 09                     	cmpb	$0x9, %bl
     1ad: 0f 87 2c ff ff ff            	ja	0xdf <rustium_parse_number_native_i64+0xdf>
     1b3: 48 8d 71 01                  	leaq	0x1(%rcx), %rsi
     1b7: 4c 39 c2                     	cmpq	%r8, %rdx
     1ba: 77 0b                        	ja	0x1c7 <rustium_parse_number_native_i64+0x1c7>
     1bc: 0f b6 fb                     	movzbl	%bl, %edi
     1bf: 75 cf                        	jne	0x190 <rustium_parse_number_native_i64+0x190>
     1c1: 40 80 ff 07                  	cmpb	$0x7, %dil
     1c5: 76 c9                        	jbe	0x190 <rustium_parse_number_native_i64+0x190>
     1c7: 31 c0                        	xorl	%eax, %eax
     1c9: 4d 85 d2                     	testq	%r10, %r10
     1cc: 0f 84 21 ff ff ff            	je	0xf3 <rustium_parse_number_native_i64+0xf3>
     1d2: 48 ff c1                     	incq	%rcx
     1d5: 4c 39 d9                     	cmpq	%r11, %rcx
     1d8: 74 1c                        	je	0x1f6 <rustium_parse_number_native_i64+0x1f6>
     1da: b8 02 00 00 00               	movl	$0x2, %eax
     1df: 0f b6 11                     	movzbl	(%rcx), %edx
     1e2: 80 c2 c6                     	addb	$-0x3a, %dl
     1e5: 80 fa f6                     	cmpb	$-0xa, %dl
     1e8: 0f 82 f1 fe ff ff            	jb	0xdf <rustium_parse_number_native_i64+0xdf>
     1ee: 48 ff c1                     	incq	%rcx
     1f1: 4c 39 d9                     	cmpq	%r11, %rcx
     1f4: 75 e9                        	jne	0x1df <rustium_parse_number_native_i64+0x1df>
     1f6: 31 c0                        	xorl	%eax, %eax
     1f8: e9 e2 fe ff ff               	jmp	0xdf <rustium_parse_number_native_i64+0xdf>
     1fd: 48 f7 da                     	negq	%rdx
     200: e9 6b fe ff ff               	jmp	0x70 <rustium_parse_number_native_i64+0x70>
     205: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x20c <rustium_parse_number_native_i64+0x20c>
     20c: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x213 <rustium_parse_number_native_i64+0x213>
     213: ba ad 01 00 00               	movl	$0x1ad, %edx            # imm = 0x1AD
     218: 45 31 c0                     	xorl	%r8d, %r8d
     21b: e8 00 00 00 00               	callq	0x220 <rustium_parse_number_native_i64+0x220>
     220: 0f 0b                        	ud2
     222: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x229 <rustium_parse_number_native_i64+0x229>
     229: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x230 <rustium_parse_number_native_i64+0x230>
     230: ba 2f 02 00 00               	movl	$0x22f, %edx            # imm = 0x22F
     235: 45 31 c0                     	xorl	%r8d, %r8d
     238: e8 00 00 00 00               	callq	0x23d <rustium_parse_number_native_i64+0x23d>
     23d: 0f 0b                        	ud2
     23f: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x246 <rustium_parse_number_native_i64+0x246>
     246: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x24d <rustium_parse_number_native_i64+0x24d>
     24d: ba b7 01 00 00               	movl	$0x1b7, %edx            # imm = 0x1B7
     252: 45 31 c0                     	xorl	%r8d, %r8d
     255: e8 00 00 00 00               	callq	0x25a <rustium_parse_number_native_i64+0x25a>
     25a: 0f 0b                        	ud2
     25c: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x263 <rustium_parse_number_native_i64+0x263>
     263: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x26a <rustium_parse_number_native_i64+0x26a>
     26a: ba b7 01 00 00               	movl	$0x1b7, %edx            # imm = 0x1B7
     26f: 45 31 c0                     	xorl	%r8d, %r8d
     272: e8 00 00 00 00               	callq	0x277 <rustium_parse_number_native_i64+0x277>
     277: 0f 0b                        	ud2

Disassembly of section .text:

0000000000000000 <rustium_parse_number_native_u32>:
       0: 56                           	pushq	%rsi
       1: 57                           	pushq	%rdi
       2: 53                           	pushq	%rbx
       3: 48 83 ec 20                  	subq	$0x20, %rsp
       7: 4c 8b 54 24 60               	movq	0x60(%rsp), %r10
       c: 48 85 d2                     	testq	%rdx, %rdx
       f: 74 4d                        	je	0x5e <rustium_parse_number_native_u32+0x5e>
      11: 48 85 c9                     	testq	%rcx, %rcx
      14: 0f 95 c0                     	setne	%al
      17: 48 85 d2                     	testq	%rdx, %rdx
      1a: 41 0f 99 c3                  	setns	%r11b
      1e: 44 84 d8                     	testb	%r11b, %al
      21: 0f 84 25 01 00 00            	je	0x14c <rustium_parse_number_native_u32+0x14c>
      27: 48 89 c8                     	movq	%rcx, %rax
      2a: 48 01 d0                     	addq	%rdx, %rax
      2d: 0f 82 fc 00 00 00            	jb	0x12f <rustium_parse_number_native_u32+0x12f>
      33: 0f b6 01                     	movzbl	(%rcx), %eax
      36: 04 d0                        	addb	$-0x30, %al
      38: 41 bb 02 00 00 00            	movl	$0x2, %r11d
      3e: 3c 09                        	cmpb	$0x9, %al
      40: 77 22                        	ja	0x64 <rustium_parse_number_native_u32+0x64>
      42: 41 83 f8 02                  	cmpl	$0x2, %r8d
      46: 41 0f 94 c0                  	sete	%r8b
      4a: 84 c0                        	testb	%al, %al
      4c: 0f 94 c3                     	sete	%bl
      4f: 41 84 d8                     	testb	%bl, %r8b
      52: 74 2c                        	je	0x80 <rustium_parse_number_native_u32+0x80>
      54: 31 c0                        	xorl	%eax, %eax
      56: 48 83 fa 01                  	cmpq	$0x1, %rdx
      5a: 75 08                        	jne	0x64 <rustium_parse_number_native_u32+0x64>
      5c: eb 2b                        	jmp	0x89 <rustium_parse_number_native_u32+0x89>
      5e: 41 bb 02 00 00 00            	movl	$0x2, %r11d
      64: 4d 85 d2                     	testq	%r10, %r10
      67: 74 0d                        	je	0x76 <rustium_parse_number_native_u32+0x76>
      69: 41 f6 c2 03                  	testb	$0x3, %r10b
      6d: 0f 85 f6 00 00 00            	jne	0x169 <rustium_parse_number_native_u32+0x169>
      73: 45 89 1a                     	movl	%r11d, (%r10)
      76: 31 c0                        	xorl	%eax, %eax
      78: 48 83 c4 20                  	addq	$0x20, %rsp
      7c: 5b                           	popq	%rbx
      7d: 5f                           	popq	%rdi
      7e: 5e                           	popq	%rsi
      7f: c3                           	retq
      80: 0f b6 c0                     	movzbl	%al, %eax
      83: 48 83 fa 01                  	cmpq	$0x1, %rdx
      87: 75 1c                        	jne	0xa5 <rustium_parse_number_native_u32+0xa5>
      89: 41 f6 c1 03                  	testb	$0x3, %r9b
      8d: 0f 94 c1                     	sete	%cl
      90: 4d 85 c9                     	testq	%r9, %r9
      93: 0f 95 c2                     	setne	%dl
      96: 84 ca                        	testb	%cl, %dl
      98: 0f 84 e8 00 00 00            	je	0x186 <rustium_parse_number_native_u32+0x186>
      9e: 41 89 01                     	movl	%eax, (%r9)
      a1: b0 01                        	movb	$0x1, %al
      a3: eb d3                        	jmp	0x78 <rustium_parse_number_native_u32+0x78>
      a5: 48 89 d6                     	movq	%rdx, %rsi
      a8: 48 f7 de                     	negq	%rsi
      ab: 41 b8 02 00 00 00            	movl	$0x2, %r8d
      b1: eb 25                        	jmp	0xd8 <rustium_parse_number_native_u32+0xd8>
      b3: 66 66 66 66 2e 0f 1f 84 00 00 00 00 00       	nopw	%cs:(%rax,%rax)
      c0: 48 8d 04 80                  	leaq	(%rax,%rax,4), %rax
      c4: 48 8d 04 47                  	leaq	(%rdi,%rax,2), %rax
      c8: 4a 8d 3c 06                  	leaq	(%rsi,%r8), %rdi
      cc: 48 ff c7                     	incq	%rdi
      cf: 49 ff c0                     	incq	%r8
      d2: 48 83 ff 01                  	cmpq	$0x1, %rdi
      d6: 74 b1                        	je	0x89 <rustium_parse_number_native_u32+0x89>
      d8: 42 0f b6 5c 01 ff            	movzbl	-0x1(%rcx,%r8), %ebx
      de: 80 c3 d0                     	addb	$-0x30, %bl
      e1: 80 fb 09                     	cmpb	$0x9, %bl
      e4: 0f 87 7a ff ff ff            	ja	0x64 <rustium_parse_number_native_u32+0x64>
      ea: 48 3d 99 99 99 19            	cmpq	$0x19999999, %rax       # imm = 0x19999999
      f0: 77 0b                        	ja	0xfd <rustium_parse_number_native_u32+0xfd>
      f2: 0f b6 fb                     	movzbl	%bl, %edi
      f5: 75 c9                        	jne	0xc0 <rustium_parse_number_native_u32+0xc0>
      f7: 40 80 ff 05                  	cmpb	$0x5, %dil
      fb: 76 c3                        	jbe	0xc0 <rustium_parse_number_native_u32+0xc0>
      fd: 31 c0                        	xorl	%eax, %eax
      ff: 4d 85 d2                     	testq	%r10, %r10
     102: 0f 84 70 ff ff ff            	je	0x78 <rustium_parse_number_native_u32+0x78>
     108: 4c 89 c0                     	movq	%r8, %rax
     10b: 48 29 d0                     	subq	%rdx, %rax
     10e: 74 17                        	je	0x127 <rustium_parse_number_native_u32+0x127>
     110: 42 0f b6 04 01               	movzbl	(%rcx,%r8), %eax
     115: 04 c6                        	addb	$-0x3a, %al
     117: 3c f6                        	cmpb	$-0xa, %al
     119: 0f 82 45 ff ff ff            	jb	0x64 <rustium_parse_number_native_u32+0x64>
     11f: 49 ff c0                     	incq	%r8
     122: 4c 39 c2                     	cmpq	%r8, %rdx
     125: 75 e9                        	jne	0x110 <rustium_parse_number_native_u32+0x110>
     127: 45 31 db                     	xorl	%r11d, %r11d
     12a: e9 35 ff ff ff               	jmp	0x64 <rustium_parse_number_native_u32+0x64>
     12f: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x136 <rustium_parse_number_native_u32+0x136>
     136: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x13d <rustium_parse_number_native_u32+0x13d>
     13d: ba ad 01 00 00               	movl	$0x1ad, %edx            # imm = 0x1AD
     142: 45 31 c0                     	xorl	%r8d, %r8d
     145: e8 00 00 00 00               	callq	0x14a <rustium_parse_number_native_u32+0x14a>
     14a: 0f 0b                        	ud2
     14c: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x153 <rustium_parse_number_native_u32+0x153>
     153: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x15a <rustium_parse_number_native_u32+0x15a>
     15a: ba 2f 02 00 00               	movl	$0x22f, %edx            # imm = 0x22F
     15f: 45 31 c0                     	xorl	%r8d, %r8d
     162: e8 00 00 00 00               	callq	0x167 <rustium_parse_number_native_u32+0x167>
     167: 0f 0b                        	ud2
     169: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x170 <rustium_parse_number_native_u32+0x170>
     170: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x177 <rustium_parse_number_native_u32+0x177>
     177: ba b7 01 00 00               	movl	$0x1b7, %edx            # imm = 0x1B7
     17c: 45 31 c0                     	xorl	%r8d, %r8d
     17f: e8 00 00 00 00               	callq	0x184 <rustium_parse_number_native_u32+0x184>
     184: 0f 0b                        	ud2
     186: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x18d <rustium_parse_number_native_u32+0x18d>
     18d: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x194 <rustium_parse_number_native_u32+0x194>
     194: ba b7 01 00 00               	movl	$0x1b7, %edx            # imm = 0x1B7
     199: 45 31 c0                     	xorl	%r8d, %r8d
     19c: e8 00 00 00 00               	callq	0x1a1 <rustium_parse_number_native_u32+0x1a1>
     1a1: 0f 0b                        	ud2

Disassembly of section .text:

0000000000000000 <rustium_parse_number_native_u64>:
       0: 56                           	pushq	%rsi
       1: 57                           	pushq	%rdi
       2: 53                           	pushq	%rbx
       3: 48 83 ec 20                  	subq	$0x20, %rsp
       7: 4c 8b 54 24 60               	movq	0x60(%rsp), %r10
       c: 48 85 d2                     	testq	%rdx, %rdx
       f: 74 5d                        	je	0x6e <rustium_parse_number_native_u64+0x6e>
      11: 48 85 c9                     	testq	%rcx, %rcx
      14: 0f 95 c0                     	setne	%al
      17: 48 85 d2                     	testq	%rdx, %rdx
      1a: 41 0f 99 c3                  	setns	%r11b
      1e: 44 84 d8                     	testb	%r11b, %al
      21: 0f 84 3e 01 00 00            	je	0x165 <rustium_parse_number_native_u64+0x165>
      27: 48 89 c8                     	movq	%rcx, %rax
      2a: 48 01 d0                     	addq	%rdx, %rax
      2d: 0f 82 15 01 00 00            	jb	0x148 <rustium_parse_number_native_u64+0x148>
      33: 0f b6 01                     	movzbl	(%rcx), %eax
      36: 04 d0                        	addb	$-0x30, %al
      38: 3c 09                        	cmpb	$0x9, %al
      3a: 76 11                        	jbe	0x4d <rustium_parse_number_native_u64+0x4d>
      3c: 45 31 c0                     	xorl	%r8d, %r8d
      3f: 31 c0                        	xorl	%eax, %eax
      41: 4d 85 d2                     	testq	%r10, %r10
      44: 74 41                        	je	0x87 <rustium_parse_number_native_u64+0x87>
      46: 45 84 c0                     	testb	%r8b, %r8b
      49: 74 28                        	je	0x73 <rustium_parse_number_native_u64+0x73>
      4b: eb 2b                        	jmp	0x78 <rustium_parse_number_native_u64+0x78>
      4d: 41 83 f8 02                  	cmpl	$0x2, %r8d
      51: 41 0f 94 c0                  	sete	%r8b
      55: 84 c0                        	testb	%al, %al
      57: 41 0f 94 c3                  	sete	%r11b
      5b: 45 84 d8                     	testb	%r11b, %r8b
      5e: 74 2f                        	je	0x8f <rustium_parse_number_native_u64+0x8f>
      60: 48 83 fa 01                  	cmpq	$0x1, %rdx
      64: 75 08                        	jne	0x6e <rustium_parse_number_native_u64+0x6e>
      66: 45 31 c0                     	xorl	%r8d, %r8d
      69: e9 bf 00 00 00               	jmp	0x12d <rustium_parse_number_native_u64+0x12d>
      6e: 4d 85 d2                     	testq	%r10, %r10
      71: 74 12                        	je	0x85 <rustium_parse_number_native_u64+0x85>
      73: b8 02 00 00 00               	movl	$0x2, %eax
      78: 41 f6 c2 03                  	testb	$0x3, %r10b
      7c: 0f 85 00 01 00 00            	jne	0x182 <rustium_parse_number_native_u64+0x182>
      82: 41 89 02                     	movl	%eax, (%r10)
      85: 31 c0                        	xorl	%eax, %eax
      87: 48 83 c4 20                  	addq	$0x20, %rsp
      8b: 5b                           	popq	%rbx
      8c: 5f                           	popq	%rdi
      8d: 5e                           	popq	%rsi
      8e: c3                           	retq
      8f: 44 0f b6 c0                  	movzbl	%al, %r8d
      93: 49 89 d3                     	movq	%rdx, %r11
      96: 49 ff cb                     	decq	%r11
      99: 0f 84 8e 00 00 00            	je	0x12d <rustium_parse_number_native_u64+0x12d>
      9f: 48 be 99 99 99 99 99 99 99 19	movabsq	$0x1999999999999999, %rsi # imm = 0x1999999999999999
      a9: 31 c0                        	xorl	%eax, %eax
      ab: eb 13                        	jmp	0xc0 <rustium_parse_number_native_u64+0xc0>
      ad: 0f 1f 00                     	nopl	(%rax)
      b0: 4f 8d 04 80                  	leaq	(%r8,%r8,4), %r8
      b4: 4e 8d 04 47                  	leaq	(%rdi,%r8,2), %r8
      b8: 48 ff c0                     	incq	%rax
      bb: 49 39 c3                     	cmpq	%rax, %r11
      be: 74 6d                        	je	0x12d <rustium_parse_number_native_u64+0x12d>
      c0: 0f b6 5c 01 01               	movzbl	0x1(%rcx,%rax), %ebx
      c5: 80 c3 d0                     	addb	$-0x30, %bl
      c8: 80 fb 09                     	cmpb	$0x9, %bl
      cb: 0f 87 6b ff ff ff            	ja	0x3c <rustium_parse_number_native_u64+0x3c>
      d1: 49 39 f0                     	cmpq	%rsi, %r8
      d4: 77 0b                        	ja	0xe1 <rustium_parse_number_native_u64+0xe1>
      d6: 0f b6 fb                     	movzbl	%bl, %edi
      d9: 75 d5                        	jne	0xb0 <rustium_parse_number_native_u64+0xb0>
      db: 40 80 ff 05                  	cmpb	$0x5, %dil
      df: 76 cf                        	jbe	0xb0 <rustium_parse_number_native_u64+0xb0>
      e1: 4d 85 d2                     	testq	%r10, %r10
      e4: 74 9f                        	je	0x85 <rustium_parse_number_native_u64+0x85>
      e6: 4c 8d 4a fe                  	leaq	-0x2(%rdx), %r9
      ea: 41 b0 01                     	movb	$0x1, %r8b
      ed: 49 39 c1                     	cmpq	%rax, %r9
      f0: 0f 84 49 ff ff ff            	je	0x3f <rustium_parse_number_native_u64+0x3f>
      f6: 48 83 c2 fd                  	addq	$-0x3, %rdx
      fa: 66 0f 1f 44 00 00            	nopw	(%rax,%rax)
     100: 44 0f b6 44 01 02            	movzbl	0x2(%rcx,%rax), %r8d
     106: 41 80 c0 d0                  	addb	$-0x30, %r8b
     10a: 41 80 f8 0a                  	cmpb	$0xa, %r8b
     10e: 41 0f 92 c0                  	setb	%r8b
     112: 41 0f 93 c1                  	setae	%r9b
     116: 48 39 c2                     	cmpq	%rax, %rdx
     119: 41 0f 94 c3                  	sete	%r11b
     11d: 45 08 cb                     	orb	%r9b, %r11b
     120: 48 ff c0                     	incq	%rax
     123: 45 84 db                     	testb	%r11b, %r11b
     126: 74 d8                        	je	0x100 <rustium_parse_number_native_u64+0x100>
     128: e9 12 ff ff ff               	jmp	0x3f <rustium_parse_number_native_u64+0x3f>
     12d: 41 f6 c1 07                  	testb	$0x7, %r9b
     131: 0f 94 c0                     	sete	%al
     134: 4d 85 c9                     	testq	%r9, %r9
     137: 0f 95 c1                     	setne	%cl
     13a: 84 c1                        	testb	%al, %cl
     13c: 74 61                        	je	0x19f <rustium_parse_number_native_u64+0x19f>
     13e: 4d 89 01                     	movq	%r8, (%r9)
     141: b0 01                        	movb	$0x1, %al
     143: e9 3f ff ff ff               	jmp	0x87 <rustium_parse_number_native_u64+0x87>
     148: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x14f <rustium_parse_number_native_u64+0x14f>
     14f: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x156 <rustium_parse_number_native_u64+0x156>
     156: ba ad 01 00 00               	movl	$0x1ad, %edx            # imm = 0x1AD
     15b: 45 31 c0                     	xorl	%r8d, %r8d
     15e: e8 00 00 00 00               	callq	0x163 <rustium_parse_number_native_u64+0x163>
     163: 0f 0b                        	ud2
     165: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x16c <rustium_parse_number_native_u64+0x16c>
     16c: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x173 <rustium_parse_number_native_u64+0x173>
     173: ba 2f 02 00 00               	movl	$0x22f, %edx            # imm = 0x22F
     178: 45 31 c0                     	xorl	%r8d, %r8d
     17b: e8 00 00 00 00               	callq	0x180 <rustium_parse_number_native_u64+0x180>
     180: 0f 0b                        	ud2
     182: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x189 <rustium_parse_number_native_u64+0x189>
     189: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x190 <rustium_parse_number_native_u64+0x190>
     190: ba b7 01 00 00               	movl	$0x1b7, %edx            # imm = 0x1B7
     195: 45 31 c0                     	xorl	%r8d, %r8d
     198: e8 00 00 00 00               	callq	0x19d <rustium_parse_number_native_u64+0x19d>
     19d: 0f 0b                        	ud2
     19f: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x1a6 <rustium_parse_number_native_u64+0x1a6>
     1a6: 4c 8d 0d 00 00 00 00         	leaq	(%rip), %r9             # 0x1ad <rustium_parse_number_native_u64+0x1ad>
     1ad: ba b7 01 00 00               	movl	$0x1b7, %edx            # imm = 0x1B7
     1b2: 45 31 c0                     	xorl	%r8d, %r8d
     1b5: e8 00 00 00 00               	callq	0x1ba <rustium_parse_number_native_u64+0x1ba>
     1ba: 0f 0b                        	ud2
