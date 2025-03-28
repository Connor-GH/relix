#include "kalloc.h"
#include "x86.h"
#include "cpu.h"
#include "mmu.h"
#include "proc.h"
#include "console.h"
#include "kernel_assert.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// bitops, cpuflags
#define _UL(x) ((unsigned long)x)
#define _BITUL(x) (_UL(1) << (x))

// I called this "XBV" instead of "xgetbv" or "xsetbv".
#define XBV_XCR0 0
enum {
	XCR0_SSE = 1 << 1,
	XCR0_AVX = 1 << 2,
};

static uint8_t clean_fpu[512];
static void
fpu_load_control_word(const uint16_t control)
{
  __asm__ __volatile__("fldcw %0;"::"m"(control));
}

static void
fpu_init(void)
{
	uint16_t fcw = 0, fsw = 0;
	uint64_t cr0 = read_cr0();

	// Needed for both fpu and SSE.
	cr0 &= ~CR0_EM;
	cr0 |= CR0_MP;
	cr0 |= CR0_NE;

	write_cr0(cr0);

	__asm__ __volatile__("fninit\t\n"
											 "fnstsw %0\t\n"
											 "fnstcw %1\t\n"
											 : "+m"(fsw), "+m"(fcw));
	fpu_load_control_word(0x37F);
	fpu_load_control_word(0x37E);
	fpu_load_control_word(0x37A);

	//__asm__ __volatile__("fxsave %0" : "=m" (clean_fpu));
}

#define PUSHF "pushfq"
#define POPF "popfq"

static bool
has_eflag(unsigned long mask)
{
	unsigned long f0 = 0, f1 = 0;

	__asm__ __volatile__(PUSHF "    \n\t" PUSHF "    \n\t"
														 "pop %0    \n\t"
														 "mov %0,%1 \n\t"
														 "xor %2,%1 \n\t"
														 "push %1   \n\t" POPF " \n\t" PUSHF "    \n\t"
														 "pop %1    \n\t" POPF
											 : "=&r"(f0), "=&r"(f1)
											 : "ri"(mask));

	return !!((f0 ^ f1) & mask);
}

static void
sse_init(CpuFeatures *features)
{

	uint64_t cr4 = read_cr4();
	cr4 |= CR4_OSFXSR;
	cr4 |= CR4_OSXMMEXCPT;
	if (features->fpu_misc.xsave)
		cr4 |= CR4_OSXSAVE;
	write_cr4(cr4);
}

static CpuFeatures
cpuflags(uint32_t cpu_vendor[static 3])
{
	uint32_t intel_level;
	CpuFeatures cpu_features = {
		/* Logically, these four exist, but we still need to check. */
		.sse = (enum SSE){ 0 },

		.avx = (enum AVX) { 0 },
		.fxsr = (enum FXSR){ 0 },
		.fpu_misc = {{0}},
	};
	/* Check whether the ID bit in rflags is supported. Support means
	 * that use of cpuid is available. */
	if (has_eflag(0x200000)) {
		cpu_features.fpu_misc.cpuid = true;
		/* call cpuid with zero */
		cpuid(0x0, 0, &intel_level, &cpu_vendor[0], &cpu_vendor[2], &cpu_vendor[1]);
	} else {
		panic("This is a 64-bit cpu without CPUID. Please send a bug report!");
	}
	return cpu_features;
}

#define MSR_MPERF 0xE7
#define MSR_APERF 0xE8
static void
do_ryzen_discovery(uint32_t ebx)
{
	pr_debug("Socket ");
	/*
	 * Bits 0-27 are reserved. 28-31 indicate the socket type.
	 */
	switch (ebx >> 28) {
	case 0x0:
		pr_debug("FP6");
		break;
	case 0x2:
		pr_debug("AM4");
		break;
	case 0x4:
		pr_debug("SP5");
		break;
	case 0x8:
		pr_debug("SP6");
		break;
	default:
		pr_debug("Unknown %d", ebx);
		break;
	}
	pr_debug("\n");
}

#define PROCESSOR_STRING_UNPACK_U32(arr, idx, reg) \
	arr[idx] = reg & 0xff; \
	arr[idx+1] = reg >> 8 & 0xff; \
	arr[idx+2] = reg >> 16 & 0xff; \
	arr[idx+3] = reg >> 24 & 0xff;
static void
model_family_stepping(void)
{
	uint32_t a, b, c, d;
	uint16_t model = 0;
	uint16_t family = 0;
	uint8_t processor_stepping = 0;
	cpuid(0x1, 0, &a, &b, &c, &d);
	processor_stepping = a & 0xF; // 15
	model = (a >> 4) & 0xF;
	family = (a >> 8) & 0xF;
	if (family == 0xF) {
		family += ((a >> 20) & 0xFF); // extended family id
	}
	if (family == 0xF || family == 0x6) {
		model += (((a >> 16) & 0xFF) << 4); // extended model
	}

	switch (family) {
	case 0x19:
		do_ryzen_discovery(b);
	}

	uint8_t bytes[49];
	for (int i = 0; i <= 2; i++) {
		cpuid(0x80000002 + i, 0, &a, &b, &c, &d);
		PROCESSOR_STRING_UNPACK_U32(bytes, 0 + i*16, a);
		PROCESSOR_STRING_UNPACK_U32(bytes, 4 + i*16, b);
		PROCESSOR_STRING_UNPACK_U32(bytes, 8 + i*16, c);
		PROCESSOR_STRING_UNPACK_U32(bytes, 12 + i*16, d);
	}
	bytes[48] = '\0';
	uart_printf("Cpu is \"%s\" (model=%x family=%x stepping=%x)\n", bytes, model, family,
							 processor_stepping);

}
static void
remaining_features(CpuFeatures *cpu_features)
{
	uint32_t a, b, c, d;
	struct cpuid_struct {
		uint32_t feature;
		const char *const feature_string;
	};
	a = b = c = d = 0;
	const struct cpuid_struct cpuidstruct_ecx_1[31] = {
		{ CPUID_FEAT_ECX_SSE3, "sse3" },
		{ CPUID_FEAT_ECX_PCLMUL, "pclmul" },
		{ CPUID_FEAT_ECX_DTES64, "dtes64" },
		{ CPUID_FEAT_ECX_MONITOR, "monitor" },
		{ CPUID_FEAT_ECX_DS_CPL, "ds_cpl" },
		{ CPUID_FEAT_ECX_VMX, "vmx" },
		{ CPUID_FEAT_ECX_SMX, "smx" },
		{ CPUID_FEAT_ECX_EST, "est" },
		{ CPUID_FEAT_ECX_TM2, "tm2" },
		{ CPUID_FEAT_ECX_SSSE3, "ssse3" },
		{ CPUID_FEAT_ECX_CID, "cid" },
		{ CPUID_FEAT_ECX_SDBG, "sdbg" },
		{ CPUID_FEAT_ECX_FMA, "fma" },
		{ CPUID_FEAT_ECX_CX16, "cx16" },
		{ CPUID_FEAT_ECX_XTPR, "xtpr" },
		{ CPUID_FEAT_ECX_PDCM, "pdcm" },
		{ CPUID_FEAT_ECX_PCID, "pcid" },
		{ CPUID_FEAT_ECX_DCA, "dca" },
		{ CPUID_FEAT_ECX_SSE4_1, "sse4_1" },
		{ CPUID_FEAT_ECX_SSE4_2, "sse4_2" },
		{ CPUID_FEAT_ECX_X2APIC, "x2apic" },
		{ CPUID_FEAT_ECX_MOVBE, "movbe" },
		{ CPUID_FEAT_ECX_POPCNT, "popcnt" },
		{ CPUID_FEAT_ECX_TSC, "tsc" },
		{ CPUID_FEAT_ECX_AES, "aes" },
		{ CPUID_FEAT_ECX_XSAVE, "xsave" },
		{ CPUID_FEAT_ECX_OSXSAVE, "osxsave" },
		{ CPUID_FEAT_ECX_AVX, "avx" },
		{ CPUID_FEAT_ECX_F16C, "f16c" },
		{ CPUID_FEAT_ECX_RDRAND, "rdrand" },
		{ CPUID_FEAT_ECX_HYPERVISOR, "hypervisor" },
	};
	const struct cpuid_struct cpuidstruct_ecx_0x80000001[31] = {
		{ CPUID_FEAT_ECX_EXT_LAHF_SAHF, "lahf/sahf" },
		{ CPUID_FEAT_ECX_EXT_CMP_LEGACY, "cmplegacy" },
		{ CPUID_FEAT_ECX_EXT_SVM, "svm" },
		{ CPUID_FEAT_ECX_EXT_EXTENDED_APIC_SPACE, "extapic" },
		{ CPUID_FEAT_ECX_EXT_ALT_MOV_CR8, "altmovcr8" },
		{ CPUID_FEAT_ECX_EXT_ABM, "lzcnt" },
		{ CPUID_FEAT_ECX_EXT_SSE4A, "sse4a" },
		{ CPUID_FEAT_ECX_EXT_MISALIGNED_SSE, "misaligned_sse" },
		{ CPUID_FEAT_ECX_EXT_3DNOW_PREFETCH, "3dnow_prefetch" },
		{ CPUID_FEAT_ECX_EXT_OSVW, "osvw" },
		{ CPUID_FEAT_ECX_EXT_IBS, "ibs" },
		{ CPUID_FEAT_ECX_EXT_XOP, "xop" },
		{ CPUID_FEAT_ECX_EXT_SKINIT, "skinit" },
		{ CPUID_FEAT_ECX_EXT_WDT, "wdt" },
		{ CPUID_FEAT_ECX_EXT_LWP, "lwp" },
		{ CPUID_FEAT_ECX_EXT_FMA4, "fma4" },
		{ CPUID_FEAT_ECX_EXT_TCE, "tce" },
		{ CPUID_FEAT_ECX_EXT_TOPEXT, "topext" },
		{ CPUID_FEAT_ECX_EXT_PERF_CTR_EXT_CORE, "perf_core" },
		{ CPUID_FEAT_ECX_EXT_PERF_CTR_EXT_DF, "perf_df" },
		{ CPUID_FEAT_ECX_EXT_DATA_BP, "data_bp" },
		{ CPUID_FEAT_ECX_EXT_PERF_TSC, "perf_tsc" },
		{ CPUID_FEAT_ECX_EXT_PERF_CTR_EXT_LLC, "perf_llc" },
		{ CPUID_FEAT_ECX_EXT_MWAITX, "mwaitx" },
		{ CPUID_FEAT_ECX_EXT_ADDR_MASK, "addr_mask" },
	};

	const struct cpuid_struct cpuidstruct_edx_1[31] = {
		{ CPUID_FEAT_EDX_FPU, "fpu" },				 { CPUID_FEAT_EDX_VME, "vme" },
		{ CPUID_FEAT_EDX_DE, "de" },					 { CPUID_FEAT_EDX_PSE, "pse" },
		{ CPUID_FEAT_EDX_TSC, "tsc" },				 { CPUID_FEAT_EDX_MSR, "msr" },
		{ CPUID_FEAT_EDX_PAE, "pae" },				 { CPUID_FEAT_EDX_MCE, "mce" },
		{ CPUID_FEAT_EDX_CX8, "cx8" },				 { CPUID_FEAT_EDX_APIC, "apic" },
		{ CPUID_FEAT_EDX_SEP, "sep" },				 { CPUID_FEAT_EDX_MTRR, "mtrr" },
		{ CPUID_FEAT_EDX_PGE, "pge" },				 { CPUID_FEAT_EDX_MCA, "mca" },
		{ CPUID_FEAT_EDX_CMOV, "cmov" },			 { CPUID_FEAT_EDX_PAT, "pat" },
		{ CPUID_FEAT_EDX_PSE36, "pse36" },		 { CPUID_FEAT_EDX_PSN, "psn" },
		{ CPUID_FEAT_EDX_CLFLUSH, "clflush" }, { CPUID_FEAT_EDX_DS, "ds" },
		{ CPUID_FEAT_EDX_ACPI, "acpi" },			 { CPUID_FEAT_EDX_MMX, "mmx" },
		{ CPUID_FEAT_EDX_FXSR, "fxsr" },			 { CPUID_FEAT_EDX_SSE, "sse" },
		{ CPUID_FEAT_EDX_SSE2, "sse2" },			 { CPUID_FEAT_EDX_SS, "ss" },
		{ CPUID_FEAT_EDX_HTT, "htt" },				 { CPUID_FEAT_EDX_TM, "tm" },
		{ CPUID_FEAT_EDX_IA64, "ia64" },			 { CPUID_FEAT_EDX_PBE, "pbe" },
	};
	const struct cpuid_struct cpuidstruct_edx_0x80000001[31] = {
		{ CPUID_FEAT_EDX_EXT_NX, "nx" },
		{ CPUID_FEAT_EDX_EXT_MMXEXT, "mmxext" },
		{ CPUID_FEAT_EDX_EXT_FFXSR, "ffxsr" },
		{ CPUID_FEAT_EDX_EXT_PAGE1GB, "page_1gb" },
		{ CPUID_FEAT_EDX_EXT_RDTSCP, "rdscp" },
		{ CPUID_FEAT_EDX_EXT_LM, "long_mode" },
		{ CPUID_FEAT_EDX_EXT_3DNOW_EXT, "3dnow_ext" },
		{ CPUID_FEAT_EDX_EXT_3DNOW, "3dnow" },
		{0}
	};

	pr_debug("Cpu features: ");
	cpuid(CPUID_EAX_GETFEATURES, 0, &a, &b, &c, &d);
	for (size_t i = 0; i < 31; i++) {
		if (c & cpuidstruct_ecx_1[i].feature && cpuidstruct_ecx_1[i].feature_string != NULL) {
			pr_debug("%s ", cpuidstruct_ecx_1[i].feature_string);
			switch (cpuidstruct_ecx_1[i].feature) {
			case CPUID_FEAT_ECX_SSE3: cpu_features->sse |= SSE3; break;
			case CPUID_FEAT_ECX_SSE4_1: cpu_features->sse |= SSE4_1; break;
			case CPUID_FEAT_ECX_SSE4_2: cpu_features->sse |= SSE4_2; break;
			case CPUID_FEAT_ECX_SSSE3: cpu_features->sse |= SSSE3; break;
			case CPUID_FEAT_ECX_AVX: cpu_features->avx |= AVX; break;
			}
		}
	}
	for (size_t i = 0; i < 31; i++) {
		if (d & cpuidstruct_edx_1[i].feature && cpuidstruct_edx_1[i].feature_string != NULL) {
			pr_debug("%s ", cpuidstruct_edx_1[i].feature_string);
			switch (cpuidstruct_edx_1[i].feature) {
			case CPUID_FEAT_EDX_FPU: cpu_features->fpu_misc.fpu = true; break;
			case CPUID_FEAT_EDX_SSE: cpu_features->sse |= SSE; break;
			case CPUID_FEAT_EDX_FXSR: cpu_features->fxsr |= FXSR; break;
			}
		}
	}
	cpuid(CPUID_EAX_GETFEATURES + 0x80000000, 0, &a, &b, &c, &d);
	for (size_t i = 0; i < 31; i++) {
		if (c & cpuidstruct_ecx_0x80000001[i].feature && cpuidstruct_ecx_0x80000001[i].feature_string != NULL) {
			pr_debug("%s ", cpuidstruct_ecx_0x80000001[i].feature_string);
			switch (cpuidstruct_ecx_1[i].feature) {
			case CPUID_FEAT_ECX_EXT_SSE4A: cpu_features->sse |= SSE4A; break;
			case CPUID_FEAT_ECX_EXT_MISALIGNED_SSE: cpu_features->sse |= SSE_MISALIGNED; break;
			case CPUID_FEAT_ECX_XSAVE: cpu_features->fpu_misc.xsave = true; break;
			}
		}
	}
	for (size_t i = 0; i < 31; i++) {
		if (d & cpuidstruct_edx_0x80000001[i].feature && cpuidstruct_edx_0x80000001[i].feature_string != NULL) {
			pr_debug("%s ", cpuidstruct_edx_0x80000001[i].feature_string);
			switch (cpuidstruct_edx_0x80000001[i].feature) {
			case CPUID_FEAT_EDX_EXT_FPU: cpu_features->fpu_misc.fpu = true; break;
			case CPUID_FEAT_EDX_EXT_FXSR: cpu_features->fxsr |= FXSR; break;
			case CPUID_FEAT_EDX_EXT_FFXSR: cpu_features->fxsr |= FFXSR; break;
			}
		}
	}
	pr_debug("\n");
}

static CpuFeatures *
set_cpu_vendor_name(char cpu_vendor_name[static 13],
										uint32_t cpu_vendor[static 3])
{
	uint32_t bitmask = 255; // 11111111
	// This memory WILL get leaked, but it lives for the
	// duration of the life of the kernel, so it is fine.
	CpuFeatures *cpu_features = kmalloc(sizeof(*cpu_features));
	CpuFeatures cpufeatures = cpuflags(cpu_vendor);
	memcpy(cpu_features, &cpufeatures, sizeof(*cpu_features));

	cpu_vendor_name[0] = (uint8_t)(cpu_vendor[0] & bitmask); // lower 7-0 bits
	cpu_vendor_name[1] =
		(uint8_t)((cpu_vendor[0] & (bitmask << 8)) >> 8); // 15-8 bits
	cpu_vendor_name[2] =
		(uint8_t)((cpu_vendor[0] & (bitmask << 16)) >> 16); // 23-16 bits
	cpu_vendor_name[3] =
		(uint8_t)((cpu_vendor[0] & (bitmask << 24)) >> 24); // 31-24 bits
	cpu_vendor_name[4] = (uint8_t)(cpu_vendor[1] & bitmask);
	cpu_vendor_name[5] = (uint8_t)((cpu_vendor[1] & (bitmask << 8)) >> 8);
	cpu_vendor_name[6] = (uint8_t)((cpu_vendor[1] & (bitmask << 16)) >> 16);
	cpu_vendor_name[7] = (uint8_t)((cpu_vendor[1] & (bitmask << 24)) >> 24);
	cpu_vendor_name[8] = (uint8_t)(cpu_vendor[2] & bitmask);
	cpu_vendor_name[9] = (uint8_t)((cpu_vendor[2] & (bitmask << 8)) >> 8);
	cpu_vendor_name[10] = (uint8_t)((cpu_vendor[2] & (bitmask << 16)) >> 16);
	cpu_vendor_name[11] = (uint8_t)((cpu_vendor[2] & (bitmask << 24)) >> 24);
	cpu_vendor_name[12] = '\0';
	return cpu_features;
}

/*
 * Contains initialization code for the FPU and SSE.
 * This may also contain code for AVX and other CPU features.
 */
void
cpu_features_init(void)
{
	char cpu_vendor_name[13];
	uint32_t cpu_vendor[3];
	CpuFeatures *cpu_features = set_cpu_vendor_name(cpu_vendor_name, cpu_vendor);
	pr_debug_file("CPU Vendor: %s\n", cpu_vendor_name);

	// We MUST have cpuid.
	kernel_assert(cpu_features->fpu_misc.cpuid);

	remaining_features(cpu_features);

	// AMD64 specifies that we have SSE, which implies FPU.
	kernel_assert(cpu_features->fpu_misc.fpu);
	fpu_init();
	kernel_assert(cpu_features->sse >= SSE && cpu_features->fxsr >= FXSR);
	sse_init(cpu_features);
	if (read_cr4() & CR4_OSXSAVE) {
		xsetbv(XBV_XCR0, xgetbv(XBV_XCR0) | XCR0_SSE);
	}

	// Wait on enabling AVX reg saving for a later date.
	#if 0
	if (cpu_features->avx >= AVX) {
		xsetbv(XBV_XCR0, xgetbv(XBV_XCR0) | XCR0_AVX);
	}
	#endif




	model_family_stepping();
}
