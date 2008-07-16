/*	$Id$	*/

/*-
 * Copyright (c) 2008 by Marco Trillo <marcotrillo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any
 * person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the
 * Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice
 * shall be included in all copies or substantial portions of
 * the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * 	void lpcm_swap16(int16_t *dstSamples, 
 *	                 const int16_t *srcSamples,
 *	                 int nSamples);
 */
	.globl	lpcm_swap16
	.align	2
lpcm_swap16:
	li	%r8, 0
1:
	cmpw	%r8, %r5
	beq	2f
	lhbrx	%r9, 0, %r4
	addi	%r8, %r8, 1
	sth	%r9, 0(%r3)
	addi	%r4, %r4, 2
	addi	%r3, %r3, 2
	b	1b
2:
	blr


/*
 *	void lpcm_swap32(int32_t *dstSamples,
 *                       const int32_t *srcSamples,
 *                       int nSamples);
 */
	.globl lpcm_swap32
	.align 2
lpcm_swap32:
	li	%r8, 0
1:
	cmpw	%r8, %r5
	beq	2f
	lwbrx	%r9, 0, %r4
	addi	%r8, %r8, 1
	stw	%r9, 0(%r3)
	addi	%r4, %r4, 4
	addi	%r3, %r3, 4
	b	1b
2:
	blr


