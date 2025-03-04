#ifndef CPU_H
#define CPU_H
#include <inttypes.h>
#include <stdbool.h>
enum SSE {
	SSE = 1 << 0,
	SSE2 = 1 << 1,
	SSE3 = 1 << 2,
	SSSE3 = 1 << 3,
	SSE4_1 = 1 << 4,
	SSE4_2 = 1 << 5,
	SSE4A = 1 << 6,
	SSE_MISALIGNED = 1 << 7,
};
enum AVX {
	AVX = 1 << 0,
	AVX2 = 1 << 1,
	AVX512 = 1 << 2,
	AVX10 = 1 << 3,
};
enum FXSR {
	// FPU xsave/xrestor
	FXSR = 1 << 0,
	// FPU fast xsave/xrestor
	FFXSR = 1 << 1,
};
typedef struct cpu_features_struct {
	enum SSE sse;
	enum AVX avx;
	enum FXSR fxsr;
	union {
		struct {
			uint32_t fpu : 1;
			uint32_t cpuid : 1;
			uint32_t mmx : 1;
			uint32_t xsave : 1;
			uint32_t osxsave : 1;

		};
		uint32_t bits;
	} fpu_misc;
} CpuFeatures;
#define CPUID_EAX_GETFEATURES 1
// from OSDEV wiki
enum {
    CPUID_FEAT_ECX_SSE3         = 1 << 0,
    CPUID_FEAT_ECX_PCLMUL       = 1 << 1,
    CPUID_FEAT_ECX_DTES64       = 1 << 2,
    CPUID_FEAT_ECX_MONITOR      = 1 << 3,
    CPUID_FEAT_ECX_DS_CPL       = 1 << 4,
    CPUID_FEAT_ECX_VMX          = 1 << 5,
    CPUID_FEAT_ECX_SMX          = 1 << 6,
    CPUID_FEAT_ECX_EST          = 1 << 7,
    CPUID_FEAT_ECX_TM2          = 1 << 8,
    CPUID_FEAT_ECX_SSSE3        = 1 << 9,
    CPUID_FEAT_ECX_CID          = 1 << 10,
    CPUID_FEAT_ECX_SDBG         = 1 << 11,
    CPUID_FEAT_ECX_FMA          = 1 << 12,
    CPUID_FEAT_ECX_CX16         = 1 << 13,
    CPUID_FEAT_ECX_XTPR         = 1 << 14,
    CPUID_FEAT_ECX_PDCM         = 1 << 15,
    CPUID_FEAT_ECX_PCID         = 1 << 17,
    CPUID_FEAT_ECX_DCA          = 1 << 18,
    CPUID_FEAT_ECX_SSE4_1       = 1 << 19,
    CPUID_FEAT_ECX_SSE4_2       = 1 << 20,
    CPUID_FEAT_ECX_X2APIC       = 1 << 21,
    CPUID_FEAT_ECX_MOVBE        = 1 << 22,
    CPUID_FEAT_ECX_POPCNT       = 1 << 23,
    CPUID_FEAT_ECX_TSC          = 1 << 24,
    CPUID_FEAT_ECX_AES          = 1 << 25,
    CPUID_FEAT_ECX_XSAVE        = 1 << 26,
    CPUID_FEAT_ECX_OSXSAVE      = 1 << 27,
    CPUID_FEAT_ECX_AVX          = 1 << 28,
    CPUID_FEAT_ECX_F16C         = 1 << 29,
    CPUID_FEAT_ECX_RDRAND       = 1 << 30,
    //CPUID_FEAT_ECX_HYPERVISOR   = 1 << 31,
#define CPUID_FEAT_ECX_HYPERVISOR (1U << 31)

		CPUID_FEAT_ECX_EXT_LAHF_SAHF  = 1 << 0,
		CPUID_FEAT_ECX_EXT_CMP_LEGACY = 1 << 1,
		CPUID_FEAT_ECX_EXT_SVM = 1 << 2,
		CPUID_FEAT_ECX_EXT_EXTENDED_APIC_SPACE = 1 << 3,
		CPUID_FEAT_ECX_EXT_ALT_MOV_CR8 = 1 << 4,
		CPUID_FEAT_ECX_EXT_ABM = 1 << 5,
		CPUID_FEAT_ECX_EXT_SSE4A = 1 << 6,
		CPUID_FEAT_ECX_EXT_MISALIGNED_SSE = 1 << 7,
		CPUID_FEAT_ECX_EXT_3DNOW_PREFETCH = 1 << 8,
		CPUID_FEAT_ECX_EXT_OSVW = 1 << 9,
		CPUID_FEAT_ECX_EXT_IBS = 1 << 10,
		CPUID_FEAT_ECX_EXT_XOP = 1 << 11,
		CPUID_FEAT_ECX_EXT_SKINIT = 1 << 12,
		CPUID_FEAT_ECX_EXT_WDT = 1 << 13,
		CPUID_FEAT_ECX_EXT_LWP = 1 << 15,
		CPUID_FEAT_ECX_EXT_FMA4 = 1 << 16,
		CPUID_FEAT_ECX_EXT_TCE = 1 << 17,
		CPUID_FEAT_ECX_EXT_TOPEXT = 1 << 22,
		CPUID_FEAT_ECX_EXT_PERF_CTR_EXT_CORE = 1 << 23,
		CPUID_FEAT_ECX_EXT_PERF_CTR_EXT_DF = 1 << 24,
		CPUID_FEAT_ECX_EXT_DATA_BP = 1 << 26,
		CPUID_FEAT_ECX_EXT_PERF_TSC = 1 << 27,
		CPUID_FEAT_ECX_EXT_PERF_CTR_EXT_LLC = 1 << 28,
		CPUID_FEAT_ECX_EXT_MWAITX = 1 << 29,
		CPUID_FEAT_ECX_EXT_ADDR_MASK = 1 << 30,

    CPUID_FEAT_EDX_FPU          = 1 << 0,
    CPUID_FEAT_EDX_VME          = 1 << 1,
    CPUID_FEAT_EDX_DE           = 1 << 2,
    CPUID_FEAT_EDX_PSE          = 1 << 3,
    CPUID_FEAT_EDX_TSC          = 1 << 4,
    CPUID_FEAT_EDX_MSR          = 1 << 5,
    CPUID_FEAT_EDX_PAE          = 1 << 6,
    CPUID_FEAT_EDX_MCE          = 1 << 7,
    CPUID_FEAT_EDX_CX8          = 1 << 8,
    CPUID_FEAT_EDX_APIC         = 1 << 9,
    CPUID_FEAT_EDX_SEP          = 1 << 11,
    CPUID_FEAT_EDX_MTRR         = 1 << 12,
    CPUID_FEAT_EDX_PGE          = 1 << 13,
    CPUID_FEAT_EDX_MCA          = 1 << 14,
    CPUID_FEAT_EDX_CMOV         = 1 << 15,
    CPUID_FEAT_EDX_PAT          = 1 << 16,
    CPUID_FEAT_EDX_PSE36        = 1 << 17,
    CPUID_FEAT_EDX_PSN          = 1 << 18,
    CPUID_FEAT_EDX_CLFLUSH      = 1 << 19,
    CPUID_FEAT_EDX_DS           = 1 << 21,
    CPUID_FEAT_EDX_ACPI         = 1 << 22,
    CPUID_FEAT_EDX_MMX          = 1 << 23,
    CPUID_FEAT_EDX_FXSR         = 1 << 24,
    CPUID_FEAT_EDX_SSE          = 1 << 25,
    CPUID_FEAT_EDX_SSE2         = 1 << 26,
    CPUID_FEAT_EDX_SS           = 1 << 27,
    CPUID_FEAT_EDX_HTT          = 1 << 28,
    CPUID_FEAT_EDX_TM           = 1 << 29,
    CPUID_FEAT_EDX_IA64         = 1 << 30,

#define CPUID_FEAT_EDX_PBE (1U << 31)

    CPUID_FEAT_EDX_EXT_FPU          = 1 << 0,
    CPUID_FEAT_EDX_EXT_VME          = 1 << 1,
    CPUID_FEAT_EDX_EXT_DE           = 1 << 2,
    CPUID_FEAT_EDX_EXT_PSE          = 1 << 3,
    CPUID_FEAT_EDX_EXT_TSC          = 1 << 4,
    CPUID_FEAT_EDX_EXT_MSR          = 1 << 5,
    CPUID_FEAT_EDX_EXT_PAE          = 1 << 6,
    CPUID_FEAT_EDX_EXT_MCE          = 1 << 7,
    CPUID_FEAT_EDX_EXT_CX8          = 1 << 8,
    CPUID_FEAT_EDX_EXT_APIC         = 1 << 9,
    CPUID_FEAT_EDX_EXT_SEP          = 1 << 11,
    CPUID_FEAT_EDX_EXT_MTRR         = 1 << 12,
    CPUID_FEAT_EDX_EXT_PGE          = 1 << 13,
    CPUID_FEAT_EDX_EXT_MCA          = 1 << 14,
    CPUID_FEAT_EDX_EXT_CMOV         = 1 << 15,
    CPUID_FEAT_EDX_EXT_PAT          = 1 << 16,
    CPUID_FEAT_EDX_EXT_PSE36        = 1 << 17,
    CPUID_FEAT_EDX_EXT_PSN          = 1 << 18,
    CPUID_FEAT_EDX_EXT_CLFLUSH      = 1 << 19,
		CPUID_FEAT_EDX_EXT_NX           = 1 << 20,
    CPUID_FEAT_EDX_EXT_DS           = 1 << 21,
    CPUID_FEAT_EDX_EXT_MMXEXT       = 1 << 22,
    CPUID_FEAT_EDX_EXT_MMX          = 1 << 23,
    CPUID_FEAT_EDX_EXT_FXSR         = 1 << 24,
    CPUID_FEAT_EDX_EXT_FFXSR        = 1 << 25,
    CPUID_FEAT_EDX_EXT_PAGE1GB      = 1 << 26,
    CPUID_FEAT_EDX_EXT_RDTSCP       = 1 << 27,
    CPUID_FEAT_EDX_EXT_HTT          = 1 << 28,
    CPUID_FEAT_EDX_EXT_LM           = 1 << 29,
    CPUID_FEAT_EDX_EXT_3DNOW_EXT    = 1 << 30,
#define CPUID_FEAT_EDX_EXT_3DNOW (1U << 31)

};

void
cpu_features_init(void);
#endif /* CPU_H */
