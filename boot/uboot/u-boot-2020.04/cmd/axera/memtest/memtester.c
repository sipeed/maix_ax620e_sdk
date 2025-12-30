/*
 * memtester version 4
 *
 * Very simple but very effective user-space memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2012 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 */
#include <common.h>
#include <command.h>
#include <stddef.h>
#include <stdio.h>
#include <asm/types.h>
#include "types.h"
#include "sizes.h"
#include "tests.h"
#include "memtester.h"

#define __version__ "4.3.0"

#define CONFIG_SYS_MEMTEST_START  0x40001000
#define CONFIG_SYS_MEMTEST_END    0xC0000000

#define EXIT_FAIL_NONSTARTER    0x01
#define EXIT_FAIL_ADDRESSLINES  0x02
#define EXIT_FAIL_OTHERTEST     0x04

struct test tests[] = {
	{ "Random Value", test_random_value },
	{ "Compare XOR", test_xor_comparison },
	{ "Compare SUB", test_sub_comparison },
	{ "Compare MUL", test_mul_comparison },
	{ "Compare DIV",test_div_comparison },
	{ "Compare OR", test_or_comparison },
	{ "Compare AND", test_and_comparison },
	{ "Sequential Increment", test_seqinc_comparison },
	{ "Solid Bits", test_solidbits_comparison },
	{ "Block Sequential", test_blockseq_comparison },
	{ "Checkerboard", test_checkerboard_comparison },
	{ "Bit Spread", test_bitspread_comparison },
	{ "Bit Flip", test_bitflip_comparison },
	{ "Walking Ones", test_walkbits1_comparison },
	{ "Walking Zeroes", test_walkbits0_comparison },
	{ "8-bit Writes", test_8bit_wide_random },
	{ "16-bit Writes", test_16bit_wide_random },
	{ NULL, NULL }
};

#if 0
/* Sanity checks and portability helper macros. */
#ifdef _SC_VERSION
void check_posix_system(void) {
	if (sysconf(_SC_VERSION) < 198808L) {
		printf("A POSIX system is required.  Don't be surprised if "
			"this craps out.\n");
		fprintf(stderr, "_SC_VERSION is %lu\n", sysconf(_SC_VERSION));
	}
}
#else
#define check_posix_system()
#endif

#ifdef _SC_PAGE_SIZE
int memtester_pagesize(void) {
	int pagesize = sysconf(_SC_PAGE_SIZE);
	if (pagesize == -1) {
		perror("get page size failed");
		exit(EXIT_FAIL_NONSTARTER);
	}
	printf("pagesize is %ld\n", (long)pagesize);
	return pagesize;
}
#else
int memtester_pagesize(void) {
	printf("sysconf(_SC_PAGE_SIZE) not supported; using pagesize of 8192\n");
	return 8192;
}
#endif

/* Some systems don't define MAP_LOCKED.  Define it to 0 here
   so it's just a no-op when ORed with other constants. */
#ifndef MAP_LOCKED
#define MAP_LOCKED 0
#endif

/* Function declarations */
void usage(char *me);

/* Function definitions */
void usage(char *me) {
	printf("\n"
			"Usage: %s [-p physaddrbase [-d device]] <mem>[B|K|M|G] [loops]\n",
			me);
}
#endif

/* Global vars - so tests have access to this information */
int use_phys = 0;
size_t physaddrbase = 0;

int do_memtest(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
	ul loops, loop, i;
	size_t bufsize, halflen, count;
	void volatile *aligned;
	ulv *bufa, *bufb;
	int exit_code = 0;
	ul testmask = 0;

	if(argc < 3) {
		printf("Usage: memtest <addr> <len> [loops]\n");
		return 0;
	}

	aligned = (void volatile *)simple_strtoul(argv[1], NULL, 16);
	bufsize = (ul)simple_strtoul(argv[2], NULL, 16);
	loops = (ul)simple_strtoul(argv[3], NULL, 10);
	if(((ul)aligned < CONFIG_SYS_MEMTEST_START) || ((ul)(aligned + bufsize) > CONFIG_SYS_MEMTEST_END)) {
		printf("available memory test region: <0x%x, 0x%x>\n", CONFIG_SYS_MEMTEST_START, CONFIG_SYS_MEMTEST_END);
		return 0;
	}

	printf("memtester version" __version__ " (%d-bit)\n", UL_LEN);
	printf("Copyright (C) 2001-2012 Charles Cazabon.\n");
	printf("Licensed under the GNU General Public License version 2 (only).\n");
	printf("\n");

	halflen = bufsize / 2;
	count = halflen / sizeof(ul);
	bufa = (ulv *) aligned;
	bufb = (ulv *) ((size_t) aligned + halflen);

	for(loop=1; ((!loops) || loop <= loops); loop++) {
		printf("Loop %lu", loop);
		if (loops) {
			printf("/%lu", loops);
		}
		printf(":\n");
		printf("  %-20s: ", "Stuck Address");
		if (!test_stuck_address(aligned, bufsize / sizeof(ul))) {
			printf("ok\n");
		} else {
			printf("test_stuck_address fail\n");
		}
		for (i=0;;i++) {
			if (!tests[i].name) break;
			/* If using a custom testmask, only run this test if the
			   bit corresponding to this test was set by the user.
			 */
			if (testmask && (!((1 << i) & testmask))) {
				continue;
			}
			printf("  %-20s: ", tests[i].name);
			if (!tests[i].fp(bufa, bufb, count)) {
				printf("ok\n");
			} else {
				exit_code |= EXIT_FAIL_OTHERTEST;
			}
		}
		printf("\n");
	}
	printf("Done.\n");

	return 0;
}

U_BOOT_CMD(
	memtest, 4, 0, do_memtest,
	"integrated memory test",
	"<addr> <len> [loops]"
);

