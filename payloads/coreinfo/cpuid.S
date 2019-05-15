/*
 * This file is part of the coreinfo project.
 *
 * It is derived from the x86info project, which is GPLv2-licensed.
 *
 * Copyright (C) 2001-2007 Dave Jones <davej@codemonkey.org.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* calling syntax:  docpuid(idx,eax,ebx,ecx,edx) */

.align 4
.text

.global docpuid
	.type docpuid,@function

docpuid:
.if 0
	pushl %ebp
	movl %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %ecx
	pushl %edx
	movl 8(%ebp),%eax
	cpuid
	movl 12(%ebp),%edi
	test %edi,%edi
	jz L1
	movl %eax, (%edi)

L1:	movl 16(%ebp),%edi
	test %edi,%edi
	jz L2
	movl %ebx,(%edi)

L2:	movl 20(%ebp),%edi
	test %edi,%edi
	jz L3
	movl %ecx,(%edi)

L3:	movl 24(%ebp), %edi
	test %edi,%edi
	jz L4
	movl %edx,(%edi)

L4:	popl %edx
	popl %ecx
	popl %ebx
	popl %edi
	movl %ebp,%esp
	popl %ebp
	ret
.else
.code64
	movq %rcx,%r9
	movq %rdx,%r10
	movq %rbx,%r11
	movq %rdi,%rax
	cpuid
	test %rsi,%rsi
	jz L1
	movl %eax, (%rsi)

L1:	test %r10,%r10
	jz L2
	movl %ebx,(%r10)

L2:	test %r9,%r9
	jz L3
	movl %ecx,(%r9)

L3:	test %r8,%r8
	jz L4
	movl %edx,(%r8)

L4:	movq %r11,%rbx
	ret
.endif

.section .note.GNU-stack,"", @progbits
