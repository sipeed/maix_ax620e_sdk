#ifndef __TYPES_H_
#define __TYPES_H_

typedef unsigned long ul;
typedef unsigned long long ull;
typedef unsigned long volatile ulv;
typedef unsigned char volatile u8v;
typedef unsigned short volatile u16v;
typedef unsigned short  u16;
typedef unsigned int  u32;
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define REG32 *(unsigned int *)
#define REG16 *(unsigned short *)

#endif

