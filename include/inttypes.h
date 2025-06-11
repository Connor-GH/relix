#pragma once
// https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/inttypes.h.html
// The <inttypes.h> header shall include the <stdint.h> header.
#include <stdint.h>

struct imaxdiv_t {};
#define __PRI64_PREFIX "l"
#define __PRIPTR_PREFIX "l"

#define PRIdMAX __PRI64_PREFIX "d"
#define PRIdPTR __PRI64_PREFIX "d"
#define PRId64 __PRI64_PREFIX "d"
#define PRId32 "d"
#define PRId16 "d"
#define PRId8 "d"

#define PRIdLEAST64 PRId64
#define PRIdLEAST32 PRId32
#define PRIdLEAST16 PRId16
#define PRIdLEAST8 PRId8

#define PRIdFAST64 PRId64
#define PRIdFAST32 PRId32
#define PRIdFAST16 PRId16
#define PRIdFAST8 PRId8

#define PRIiMAX __PRI64_PREFIX "i"
#define PRIiPTR __PRI64_PREFIX "i"
#define PRIi64 __PRI64_PREFIX "i"
#define PRIi32 "i"
#define PRIi16 "i"
#define PRIi8 "i"

#define PRIiLEAST64 PRIi64
#define PRIiLEAST32 PRIi32
#define PRIiLEAST16 PRIi16
#define PRIiLEAST8 PRIi8

#define PRIiFAST64 PRIi64
#define PRIiFAST32 PRIi32
#define PRIiFAST16 PRIi16
#define PRIiFAST8 PRIi8

#define PRIoMAX __PRI64_PREFIX "o"
#define PRIoPTR __PRI64_PREFIX "o"
#define PRIo64 __PRI64_PREFIX "o"
#define PRIo32 "o"
#define PRIo16 "o"
#define PRIo8 "o"

#define PRIoLEAST64 PRIo64
#define PRIoLEAST32 PRIo32
#define PRIoLEAST16 PRIo16
#define PRIoLEAST8 PRIo8

#define PRIoFAST64 PRIo64
#define PRIoFAST32 PRIo32
#define PRIoFAST16 PRIo16
#define PRIoFAST8 PRIo8

#define PRIxMAX __PRI64_PREFIX "x"
#define PRIxPTR __PRI64_PREFIX "x"
#define PRIx64 __PRI64_PREFIX "x"
#define PRIx32 "x"
#define PRIx16 "x"
#define PRIx8 "x"

#define PRIxLEAST64 PRIx64
#define PRIxLEAST32 PRIx32
#define PRIxLEAST16 PRIx16
#define PRIxLEAST8 PRIx8

#define PRIxFAST64 PRIx64
#define PRIxFAST32 PRIx32
#define PRIxFAST16 PRIx16
#define PRIxFAST8 PRIx8

#define PRIXMAX __PRI64_PREFIX "X"
#define PRIXPTR __PRI64_PREFIX "X"
#define PRIX64 __PRI64_PREFIX "X"
#define PRIX32 "X"
#define PRIX16 "X"
#define PRIX8 "X"

#define PRIXLEAST64 PRIX64
#define PRIXLEAST32 PRIX32
#define PRIXLEAST16 PRIX16
#define PRIXLEAST8 PRIX8

#define PRIXFAST64 PRIX64
#define PRIXFAST32 PRIX32
#define PRIXFAST16 PRIX16
#define PRIXFAST8 PRIX8

#define PRIuMAX __PRI64_PREFIX "u"
#define PRIuPTR __PRI64_PREFIX "u"
#define PRIu64 __PRI64_PREFIX "u"
#define PRIu32 "u"
#define PRIu16 "u"
#define PRIu8 "u"

#define PRIuLEAST64 PRIu64
#define PRIuLEAST32 PRIu32
#define PRIuLEAST16 PRIu16
#define PRIuLEAST8 PRIu8

#define PRIuFAST64 PRIu64
#define PRIuFAST32 PRIu32
#define PRIuFAST16 PRIu16
#define PRIuFAST8 PRIu8
