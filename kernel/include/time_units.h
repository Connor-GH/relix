#pragma once

#ifndef USE_HOST_TOOLS

#define MSEC_PER_SEC 1000LU
#define USEC_PER_SEC 1000000LU
#define NSEC_PER_SEC 1000000000LU
#define FSEC_PER_SEC 1000000000000000LU

#define sec_to_msec(x) ((x) * 1000LU)
#define msec_to_usec(x) ((x) * 1000LU)
#define usec_to_nsec(x) ((x) * 1000LU)
#define nsec_to_fsec(x) ((x) * 1000000LU)

#define fsec_to_nsec(x) ((x) / 1000000LU)
#define nsec_to_usec(x) ((x) / 1000LU)
#define usec_to_msec(x) ((x) / 1000LU)
#define msec_to_sec(x) ((x) / 1000LU)
#endif
