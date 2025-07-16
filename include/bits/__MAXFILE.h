#pragma once
#define __NDIRECT 8UL
#define __NINDIRECT (__BSIZE / sizeof(uintptr_t))
#define __MAXFILE (__NDIRECT + __NINDIRECT + (__NINDIRECT * __NINDIRECT))
#define __NDINDIRECT_PER_ENTRY __NDIRECT
#define __NDINDIRECT_ENTRY __NDIRECT
