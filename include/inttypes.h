#pragma once
// https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/inttypes.h.html
// The <inttypes.h> header shall include the <stdint.h> header.
#include <bits/wchar_t.h>
#include <stdint.h>

struct imaxdiv_t {};
typedef __wchar_t wchar_t;
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

#define __SCN64_PREFIX "l"
#define __SCNPTR_PREFIX "l"

#define SCNdMAX __SCN64_PREFIX "d"
#define SCNdPTR __SCN64_PREFIX "d"
#define SCNd64 __SCN64_PREFIX "d"
#define SCNd32 "d"
#define SCNd16 "d"
#define SCNd8 "d"

#define SCNdLEAST64 SCNd64
#define SCNdLEAST32 SCNd32
#define SCNdLEAST16 SCNd16
#define SCNdLEAST8 SCNd8

#define SCNdFAST64 SCNd64
#define SCNdFAST32 SCNd32
#define SCNdFAST16 SCNd16
#define SCNdFAST8 SCNd8

#define SCNiMAX __SCN64_PREFIX "i"
#define SCNiPTR __SCN64_PREFIX "i"
#define SCNi64 __SCN64_PREFIX "i"
#define SCNi32 "i"
#define SCNi16 "i"
#define SCNi8 "i"

#define SCNiLEAST64 SCNi64
#define SCNiLEAST32 SCNi32
#define SCNiLEAST16 SCNi16
#define SCNiLEAST8 SCNi8

#define SCNiFAST64 SCNi64
#define SCNiFAST32 SCNi32
#define SCNiFAST16 SCNi16
#define SCNiFAST8 SCNi8

#define SCNoMAX __SCN64_PREFIX "o"
#define SCNoPTR __SCN64_PREFIX "o"
#define SCNo64 __SCN64_PREFIX "o"
#define SCNo32 "o"
#define SCNo16 "o"
#define SCNo8 "o"

#define SCNoLEAST64 SCNo64
#define SCNoLEAST32 SCNo32
#define SCNoLEAST16 SCNo16
#define SCNoLEAST8 SCNo8

#define SCNoFAST64 SCNo64
#define SCNoFAST32 SCNo32
#define SCNoFAST16 SCNo16
#define SCNoFAST8 SCNo8

#define SCNxMAX __SCN64_PREFIX "x"
#define SCNxPTR __SCN64_PREFIX "x"
#define SCNx64 __SCN64_PREFIX "x"
#define SCNx32 "x"
#define SCNx16 "x"
#define SCNx8 "x"

#define SCNxLEAST64 SCNx64
#define SCNxLEAST32 SCNx32
#define SCNxLEAST16 SCNx16
#define SCNxLEAST8 SCNx8

#define SCNxFAST64 SCNx64
#define SCNxFAST32 SCNx32
#define SCNxFAST16 SCNx16
#define SCNxFAST8 SCNx8

#define SCNXMAX __SCN64_PREFIX "X"
#define SCNXPTR __SCN64_PREFIX "X"
#define SCNX64 __SCN64_PREFIX "X"
#define SCNX32 "X"
#define SCNX16 "X"
#define SCNX8 "X"

#define SCNXLEAST64 SCNX64
#define SCNXLEAST32 SCNX32
#define SCNXLEAST16 SCNX16
#define SCNXLEAST8 SCNX8

#define SCNXFAST64 SCNX64
#define SCNXFAST32 SCNX32
#define SCNXFAST16 SCNX16
#define SCNXFAST8 SCNX8

#define SCNuMAX __SCN64_PREFIX "u"
#define SCNuPTR __SCN64_PREFIX "u"
#define SCNu64 __SCN64_PREFIX "u"
#define SCNu32 "u"
#define SCNu16 "u"
#define SCNu8 "u"

#define SCNuLEAST64 SCNu64
#define SCNuLEAST32 SCNu32
#define SCNuLEAST16 SCNu16
#define SCNuLEAST8 SCNu8

#define SCNuFAST64 SCNu64
#define SCNuFAST32 SCNu32
#define SCNuFAST16 SCNu16
#define SCNuFAST8 SCNu8
