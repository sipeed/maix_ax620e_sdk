/*
 * Very simple but very effective user-space memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2012 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 * This file contains the functions for the actual tests, called from the
 * main routine in memtester.c.  See other comments in that file.
 *
 */

#include <stdio.h>
#include <common.h>
#include "types.h"
#include "sizes.h"

/* extern declarations. */
extern int use_phys;
extern off_t physaddrbase;

char progress[] = "-\\|/";
#define PROGRESSLEN 4
#define PROGRESSOFTEN 2500
#define ONE 0x00000001L

union {
	unsigned char bytes[UL_LEN/8];
	ul val;
} mword8;
union {
	unsigned short u16s[UL_LEN/16];
	ul val;
} mword16;

/* Function definitions. */
int compare_regions(ulv *bufa, ulv *bufb, size_t count) {
	int r = 0;
	size_t i;
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	off_t physaddr;
	int retry_time = 10;
	ul src, dst;

	for (i = 0; i < count; i++, p1++, p2++) {
compare_retry:
		src = (ul) * p1;
		dst = (ul) * p2;
		if (src != dst) {
			if (use_phys) {
				physaddr = physaddrbase + (i * sizeof(ul));
				printf(
					"FAILURE: 0x%lx != 0x%lx at physical address "
					"0x%lx.\n",
					(ul) *p1, (ul) *p2, physaddr);
			} else {
				printf(
					"FAILURE: 0x%lx != 0x%lx at offset 0x%lx.\n",
					src, dst, (ul)(i * sizeof(ul)));
				if (retry_time > 0) {
					retry_time--;
					printf("Re-read time %d: ", retry_time);
					goto compare_retry;
				} else if (retry_time == 0) {
					retry_time = 10;
				}
			}
			/* printf("Skipping to next test..."); */
			r = -1;
		}
	}
	return r;
}

int test_stuck_address(ulv *bufa, size_t count) {
	ulv *p1 = bufa;
	unsigned int j;
	size_t i;
	off_t physaddr;
	int retry_time = 10;
	ul val;

	//printf("           ");
	for (j = 0; j < 16; j++) {
		//printf("\b\b\b\b\b\b\b\b\b\b\b");
		p1 = (ulv *) bufa;
		//printf("setting %3u", j);
		for (i = 0; i < count; i++) {
			*p1 = ((j + i) % 2) == 0 ? (ul) p1 : ~((ul) p1);
			*p1++;
		}
		//printf("\b\b\b\b\b\b\b\b\b\b\b");
		//printf("testing %3u", j);
		p1 = (ulv *) bufa;
		for (i = 0; i < count; i++, p1++) {
stuck_retry:
			val = *p1;
			if (val != (((j + i) % 2) == 0 ? (ul) p1 : ~((ul) p1))) {
				if (use_phys) {
					physaddr = physaddrbase + (i * sizeof(ul));
					printf(
						"FAILURE: possible bad address line at physical "
						"address 0x%lx.\n",
						physaddr);
				} else {
					printf(
						"FAILURE: possible bad address line at offset "
						"0x%lx, expect 0x%lx actul 0x%lx\n",
						(ul)(i * sizeof(ul)), (((j + i) % 2) == 0 ? (ul) p1 : ~((ul) p1)), val);
					if (retry_time > 0) {
						retry_time--;
						printf("Re-read time %d: ", retry_time);
						goto stuck_retry;
					} else if (retry_time == 0) {
						retry_time = 10;
					}
				}
				printf("Skipping to next test...\n");
				return -1;
			}
		}
	}
	//printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
	return 0;
}

int test_random_value(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	//ul j = 0;
	size_t i;

	//printf(" ");
	for (i = 0; i < count; i++) {
		*p1++ = *p2++ = rand_ul();
		/*
		if (!(i % PROGRESSOFTEN)) {
			printf("\b");
			printf("%c", progress[++j % PROGRESSLEN]);
		}
		*/
	}
	//printf("\b \b");
	return compare_regions(bufa, bufb, count);
}

int test_xor_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;
	ul q = rand_ul();

	for (i = 0; i < count; i++) {
		*p1++ ^= q;
		*p2++ ^= q;
	}
	return compare_regions(bufa, bufb, count);
}

int test_sub_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;
	ul q = rand_ul();

	for (i = 0; i < count; i++) {
		*p1++ -= q;
		*p2++ -= q;
	}
	return compare_regions(bufa, bufb, count);
}

int test_mul_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;
	ul q = rand_ul();

	for (i = 0; i < count; i++) {
		*p1++ *= q;
		*p2++ *= q;
	}
	return compare_regions(bufa, bufb, count);
}

int test_div_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;
	ul q = rand_ul();

	for (i = 0; i < count; i++) {
		if (!q) {
			q++;
		}
		*p1++ /= q;
		*p2++ /= q;
	}
	return compare_regions(bufa, bufb, count);
}

int test_or_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;
	ul q = rand_ul();

	for (i = 0; i < count; i++) {
		*p1++ |= q;
		*p2++ |= q;
	}
	return compare_regions(bufa, bufb, count);
}

int test_and_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;
	ul q = rand_ul();

	for (i = 0; i < count; i++) {
		*p1++ &= q;
		*p2++ &= q;
	}
	return compare_regions(bufa, bufb, count);
}

int test_seqinc_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;
	ul q = rand_ul();

	for (i = 0; i < count; i++) {
		*p1++ = *p2++ = (i + q);
	}
	return compare_regions(bufa, bufb, count);
}

int test_solidbits_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j;
	ul q;
	size_t i;

	//printf("           ");
	for (j = 0; j < 64; j++) {
		//printf("\b\b\b\b\b\b\b\b\b\b\b");
		q = (j % 2) == 0 ? UL_ONEBITS : 0;
		//printf("setting %3u", j);
		p1 = (ulv *) bufa;
		p2 = (ulv *) bufb;
		for (i = 0; i < count; i++) {
			*p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
		}
		//printf("\b\b\b\b\b\b\b\b\b\b\b");
		//printf("testing %3u", j);
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	//printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
	return 0;
}

int test_checkerboard_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j;
	ul q;
	size_t i;

	//printf("           ");
	for (j = 0; j < 64; j++) {
		//printf("\b\b\b\b\b\b\b\b\b\b\b");
		q = (j % 2) == 0 ? CHECKERBOARD1 : CHECKERBOARD2;
		//printf("setting %3u", j);
		p1 = (ulv *) bufa;
		p2 = (ulv *) bufb;
		for (i = 0; i < count; i++) {
			*p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
		}
		//printf("\b\b\b\b\b\b\b\b\b\b\b");
		//printf("testing %3u", j);
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	//printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
	return 0;
}

int test_blockseq_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j;
	size_t i;

	//printf("           ");
	for (j = 0; j < 256; j++) {
		//printf("\b\b\b\b\b\b\b\b\b\b\b");
		p1 = (ulv *) bufa;
		p2 = (ulv *) bufb;
		//printf("setting %3u", j);
		for (i = 0; i < count; i++) {
			*p1++ = *p2++ = (ul) UL_BYTE(j);
		}
		//printf("\b\b\b\b\b\b\b\b\b\b\b");
		//printf("testing %3u", j);
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	//printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
	return 0;
}

int test_walkbits0_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j;
	size_t i;

	//printf("           ");
	for (j = 0; j < UL_LEN * 2; j++) {
		//printf("\b\b\b\b\b\b\b\b\b\b\b");
		p1 = (ulv *) bufa;
		p2 = (ulv *) bufb;
		//printf("setting %3u", j);
		for (i = 0; i < count; i++) {
			if (j < UL_LEN) { /* Walk it up. */
				*p1++ = *p2++ = ONE << j;
			} else { /* Walk it back down. */
				*p1++ = *p2++ = ONE << (UL_LEN * 2 - j - 1);
			}
		}
		//printf("\b\b\b\b\b\b\b\b\b\b\b");
		//printf("testing %3u", j);
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	//printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
	return 0;
}

int test_walkbits1_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j;
	size_t i;

	//printf("           ");
	for (j = 0; j < UL_LEN * 2; j++) {
		//printf("\b\b\b\b\b\b\b\b\b\b\b");
		p1 = (ulv *) bufa;
		p2 = (ulv *) bufb;
		//printf("setting %3u", j);
		for (i = 0; i < count; i++) {
			if (j < UL_LEN) { /* Walk it up. */
				*p1++ = *p2++ = UL_ONEBITS ^ (ONE << j);
			} else { /* Walk it back down. */
				*p1++ = *p2++ = UL_ONEBITS ^ (ONE << (UL_LEN * 2 - j - 1));
			}
		}
		//printf("\b\b\b\b\b\b\b\b\b\b\b");
		//printf("testing %3u", j);
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	//printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
	return 0;
}

int test_bitspread_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j;
	size_t i;

	//printf("           ");
	for (j = 0; j < UL_LEN * 2; j++) {
		//printf("\b\b\b\b\b\b\b\b\b\b\b");
		p1 = (ulv *) bufa;
		p2 = (ulv *) bufb;
		//printf("setting %3u", j);
		for (i = 0; i < count; i++) {
			if (j < UL_LEN) { /* Walk it up. */
				*p1++ = *p2++ = (i % 2 == 0)
					? (ONE << j) | (ONE << (j + 2))
					: UL_ONEBITS ^ ((ONE << j)
									| (ONE << (j + 2)));
			} else { /* Walk it back down. */
				*p1++ = *p2++ = (i % 2 == 0)
					? (ONE << (UL_LEN * 2 - 1 - j)) | (ONE << (UL_LEN * 2 + 1 - j))
					: UL_ONEBITS ^ (ONE << (UL_LEN * 2 - 1 - j)
									| (ONE << (UL_LEN * 2 + 1 - j)));
			}
		}
		//printf("\b\b\b\b\b\b\b\b\b\b\b");
		//printf("testing %3u", j);
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	//printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
	return 0;
}

int test_bitflip_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j, k;
	ul q;
	size_t i;

	//printf("           ");
	for (k = 0; k < UL_LEN; k++) {
		q = ONE << k;
		for (j = 0; j < 8; j++) {
			//printf("\b\b\b\b\b\b\b\b\b\b\b");
			q = ~q;
			//printf("setting %3u", k * 8 + j);
			p1 = (ulv *) bufa;
			p2 = (ulv *) bufb;
			for (i = 0; i < count; i++) {
				*p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
			}
			//printf("\b\b\b\b\b\b\b\b\b\b\b");
			//printf("testing %3u", k * 8 + j);
			if (compare_regions(bufa, bufb, count)) {
				return -1;
			}
		}
	}
	//printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
	return 0;
}

int test_8bit_wide_random(ulv* bufa, ulv* bufb, size_t count) {
	u8v *p1, *t;
	ulv *p2;
	int attempt;
	unsigned int b/*, j = 0*/;
	size_t i;

	//printf(".");
	for (attempt = 0; attempt < 2;  attempt++) {
		if (attempt & 1) {
			p1 = (u8v *) bufa;
			p2 = bufb;
		} else {
			p1 = (u8v *) bufb;
			p2 = bufa;
		}
		for (i = 0; i < count; i++) {
			t = mword8.bytes;
			*p2++ = mword8.val = rand_ul();
			for (b = 0; b < UL_LEN / 8; b++) {
				*p1++ = *t++;
			}
			/*
			if (!(i % PROGRESSOFTEN)) {
				printf("\b");
				printf("%c", progress[++j % PROGRESSLEN]);
			}
			*/
		}
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	//printf("\b \b");
	return 0;
}

int test_16bit_wide_random(ulv* bufa, ulv* bufb, size_t count) {
	u16v *p1, *t;
	ulv *p2;
	int attempt;
	unsigned int b/*, j = 0*/;
	size_t i;

	//printf(".");
	for (attempt = 0; attempt < 2; attempt++) {
		if (attempt & 1) {
			p1 = (u16v *) bufa;
			p2 = bufb;
		} else {
			p1 = (u16v *) bufb;
			p2 = bufa;
		}
		for (i = 0; i < count; i++) {
			t = mword16.u16s;
			*p2++ = mword16.val = rand_ul();
			for (b = 0; b < UL_LEN / 16; b++) {
				*p1++ = *t++;
			}
			/*
			if (!(i % PROGRESSOFTEN)) {
				printf("\b");
				printf("%c", progress[++j % PROGRESSLEN]);
			}
			*/
		}
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	//printf("\b \b");
	return 0;
}
