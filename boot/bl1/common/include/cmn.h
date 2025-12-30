#ifndef __CMN_H
#define __CMN_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long size_t;
typedef unsigned long long u64;

u8 readb(unsigned long addr);
u16 readw(unsigned long addr);
u32 readl(unsigned long addr);
void writeb(u8 value, unsigned long addr);
void writew(u16 value, unsigned long addr);
void writel(u32 value, unsigned long addr);
void *ax_memset(void *start, u8 value, u32 len);
void *ax_memcpy(void *dest, const void *src, u32 count);
u32 ax_strlen(const char *s);
int dma_memcpy(unsigned long dst, unsigned long src, int size);
int dma_memset(unsigned long dst, unsigned long value, int size);
int memcmp(const void * cs, const void * ct, int count);
void *(memcpy)(void *__dest, __const void *__src, size_t __n);
void *(memmove)(void *__dest, __const void *__src, size_t __n);
void *(memchr)(void const *s, int c, size_t n);
size_t (strlen)(const char *s);
void *(memset)(void *s, int c, size_t count);
int (strcmp)(char const *s1, char const *s2);
int (strncmp)(char const *s1, char const *s2, size_t n);
char *(strchr)(char const *s, int c);
int ffs(int x);

#define UNUSED(x)	(void)x
#define NULL		(void *)0

#define BITS_PER_INT				32	/*64-bit CPU, sizeof(int)=4 */
#define BITS_PER_LONG				64	/*64-bit CPU, sizeof(long)=8 */

#define BIT(nr)						(1U << (nr))
#define BIT_UL(nr)					(1UL << (nr))
#define BIT_MASK(nr)				(1U << ((nr) % BITS_PER_INT))
#define BIT_UL_MASK(nr)				(1UL << ((nr) % BITS_PER_LONG))
#define BIT_UL_WORD(nr)				((nr) / BITS_PER_LONG)
#define BITS_PER_BYTE				8

/*
 * Create a contiguous bitmask starting at bit position @l and ending at
 * position @h. For example
 * GENMASK_UL(39, 21) gives us the 64bit vector 0x000000ffffe00000.
 */
#define GENMASK(h, l) \
	(((~0U) << (l)) & (~0U >> (BITS_PER_INT - 1 - (h))))

#define GENMASK_ULL(h, l) \
	(((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

//#define SAMPLE_EFUSE_WRITE
#ifdef SAMPLE_EFUSE_WRITE
#define EFSC_WRITE_ENABLE
#endif

#endif
