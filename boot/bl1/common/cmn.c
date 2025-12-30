#include "cmn.h"
#include "axdma_api.h"
#include "chip_reg.h"
u8 readb(unsigned long addr)
{
	return *(const volatile u8 *)addr;
}

u16 readw(unsigned long addr)
{
	return *(const volatile u16 *)addr;
}

u32 readl(unsigned long addr)
{
	return *(const volatile u32 *)addr;
}

void writeb(u8 value, unsigned long addr)
{
	*(volatile u8 *)addr = value;
}

void writew(u16 value, unsigned long addr)
{
	*(volatile u16 *)addr = value;
}

void writel(u32 value, unsigned long addr)
{
	*(volatile u32 *)addr = value;
}

void *ax_memset(void *start, u8 value, u32 len)
{
	u8 *dest = start;

	//may optimize later
	while (len--)
		*dest++ = value;

	return start;
}

void *ax_memcpy(void *dest, const void *src, u32 count)
{
	char *tmp = dest;
	const char *s = src;

	//may optimize later
	while (count--)
		*tmp++ = *s++;
	return dest;
}

u32 ax_strlen(const char *s)
{
	const char *sc;

	for (sc = s; *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}

/* please make sure axi_dma_hw_init() has been called before dma_memcpy() & dma_memset() */
/**
 * memcmp - Compare two areas of memory
 * @cs: One area of memory
 * @ct: Another area of memory
 * @count: The size of the area.
 */
int memcmp(const void * cs, const void * ct, int count)
{
	const unsigned char *su1, *su2;
	int res = 0;

	for( su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;
}

int ffs(int x)
{
	return __builtin_ffs(x);
}

int fls(int x)
{
	return x ? 32 - __builtin_clz(x) : 0;
}

static void *__memmove_down(void *__dest, __const void *__src, size_t __n)
{
	unsigned char *d = (unsigned char *)__dest, *s = (unsigned char *)__src;

	while (__n--)
		*d++ = *s++;

	return __dest;
}

static void *__memmove_up(void *__dest, __const void *__src, size_t __n)
{
	unsigned char *d = (unsigned char *)__dest + __n - 1, *s = (unsigned char *)__src + __n - 1;

	while (__n--)
		*d-- = *s--;

	return __dest;
}

void *(memcpy)(void *__dest, __const void *__src, size_t __n)
{
	return __memmove_down(__dest, __src, __n);
}

void *(memmove)(void *__dest, __const void *__src, size_t __n)
{
	if(__dest > __src)
		return __memmove_up(__dest, __src, __n);
	else
		return __memmove_down(__dest, __src, __n);
}

void *(memchr)(void const *s, int c, size_t n)
{
	unsigned char const *_s = (unsigned char const *)s;

	while(n && *_s != c) {
		++_s;
		--n;
	}

	if(n)
		return (void *)_s;	/* the C library casts const away */
	else
		return (void *)0;
}

size_t (strlen)(const char *s)
{
	const char *sc = s;

	while (*sc != '\0')
		sc++;
	return sc - s;
}

void *(memset)(void *s, int c, size_t count)
{
	char *xs = s;
	while (count--)
		*xs++ = c;
	return s;
}

int (strcmp)(char const *s1, char const *s2)
{
	while(*s1 && *s2) {
		if(*s1 < *s2)
			return -1;
		else if(*s1 > *s2)
			return 1;

		++s1;
		++s2;
	}

	if(!*s1 && !*s2)
		return 0;
	else if(!*s1)
		return -1;
	else
		return 1;
}

int (strncmp)(char const *s1, char const *s2, size_t n)
{
	while(*s1 && *s2 && n--) {
		if(*s1 < *s2)
			return -1;
		else if(*s1 > *s2)
			return 1;

		++s1;
		++s2;
	}

	if(n == 0 || (!*s1 && !*s2))
		return 0;
	else if(!*s1)
		return -1;
	else
		return 1;
}

char *(strchr)(char const *s, int c)
{
	unsigned char const *_s = (unsigned char const *)s;

	while(*_s && *_s != c)
		++_s;

	if(*_s)
		return (char *)_s;	/* the C library casts const away */
	else
		return (char *)0;
}

int raise(void)
{
	return 0;
}
