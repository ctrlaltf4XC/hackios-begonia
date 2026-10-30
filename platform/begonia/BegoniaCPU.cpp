/*
 * A55 SMP bring-up for begonia (MT6785) using XNU's legacy IOCPU interface
 * and PSCI. The architected timer PPI 13 and SGI1 enter through pe_fiq/
 * sleh_irq. CPU_OFF, suspend and the A76 cores still require separate
 * platform work.
 *
 * POX DTS references (arch/arm64/boot/dts/mediatek/mt6785.dts @ 2fb79cd9cba6):
 *   cpus: cpu0-cpu5 = cluster0 (A55), cpu6-cpu7 = cluster1 (DTS says A75;
 *         target spec says A76; probe MIDR_EL1 at boot)
 *   cpu-map: cluster0 = cpu0..cpu5, cluster1 = cpu6..cpu7  (mt6785.dts:131-165)
 *   PSCI: arm,psci-1.0 method=smc  (mt6785.dts:263-266)
 *   armv8-timer: virtual timer PPI 13  (mt6785.dts:779)
 *
 * SMCCC 1.0 PSCI CPU_ON: x0 = 0xC4000003, x1 = target_affinity,
 *   x2 = entry_point, x3 = context_id. Result in x0 (signed).
 */
#include <pexpert/arm64/board_config.h>

#if defined(BEGONIA)
#include <IOKit/IOCPU.h>
#include <IOKit/IOLib.h>
#include <libkern/c++/OSData.h>
#include <libkern/c++/OSNumber.h>
#include <arm64/proc_reg.h>
#include <machine/atomic.h>
#include <kern/clock.h>
#include <kern/processor.h>
#include <kern/thread.h>
#include <mach/mach_time.h>
#include <pexpert/pexpert.h>

extern "C" void begonia_secondary_entry(void);
extern "C" boolean_t begonia_cpu_signal_is_enabled(unsigned int cpu);

class BegoniaCPU;
static BegoniaCPU *begonia_cpus[BEGONIA_MAX_CPUS];
static uint32_t begonia_cpus_registered;

/* SMCCC 1.0: x0 is the signed PSCI result; do not assume x1-x17 survive. */
static int32_t
begonia_psci_cpu_on(uint64_t target, uint64_t entry, uint64_t context)
{
	register uint64_t x0 __asm__("x0") = 0xC4000003ULL;
	register uint64_t x1 __asm__("x1") = target;
	register uint64_t x2 __asm__("x2") = entry;
	register uint64_t x3 __asm__("x3") = context;
	__asm__ volatile("smc #0" : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3) : :
	    "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13",
	    "x14", "x15", "x16", "x17", "cc", "memory");
	return (int32_t)x0;
}

class BegoniaCPU : public IOCPU {
	OSDeclareDefaultStructors(BegoniaCPU);
	const OSSymbol *cpuName = NULL;
	uint32_t initialized = 0; /* release/acquire publication after initCPU */

public:
	static void startAll(void *arg, wait_result_t result);
	bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
	void free(void) APPLE_KEXT_OVERRIDE;
	void initCPU(bool boot) APPLE_KEXT_OVERRIDE;
	void quiesceCPU(void) APPLE_KEXT_OVERRIDE;
	kern_return_t startCPU(vm_offset_t start_paddr, vm_offset_t arg_paddr) APPLE_KEXT_OVERRIDE;
	void haltCPU(void) APPLE_KEXT_OVERRIDE;
	void signalCPU(IOCPU *target) APPLE_KEXT_OVERRIDE;
	const OSSymbol *getCPUName(void) APPLE_KEXT_OVERRIDE;
};

OSDefineMetaClassAndStructors(BegoniaCPU, IOCPU);

bool
BegoniaCPU::start(IOService *provider)
{
	const ml_topology_info_t *topology = ml_get_topology_info();
	OSNumber *logical = OSDynamicCast(OSNumber, provider->getProperty("logical-cpu-id"));
	OSData *reg = OSDynamicCast(OSData, provider->getProperty("reg"));

	if (logical == NULL || reg == NULL || reg->getLength() != sizeof(uint32_t) ||
	    logical->unsigned32BitValue() >= topology->num_cpus) {
		return false;
	}
	const ml_topology_cpu_t *cpu = &topology->cpus[logical->unsigned32BitValue()];
	if (*(const uint32_t *)reg->getBytesNoCopy() != cpu->phys_id) {
		return false;
	}
	if (cpu->phys_id > 0x500 || (cpu->phys_id & 0xff) != 0 ||
	    topology->num_cpus > BEGONIA_MAX_CPUS) {
		panic("BEGONIA CPU: invalid topology cpu=%u mpidr=0x%x count=%u",
		    cpu->cpu_id, cpu->phys_id, topology->num_cpus);
	}
	if (!IOCPU::start(provider)) {
		return false;
	}
	registerService();
	__atomic_store_n(&begonia_cpus[cpu->cpu_id], this, __ATOMIC_RELEASE);
	__atomic_add_fetch(&begonia_cpus_registered, 1, __ATOMIC_ACQ_REL);
	if (cpu == topology->boot_cpu) {
		thread_t thread;
		kern_return_t result = kernel_thread_start(&BegoniaCPU::startAll, NULL, &thread);
		if (result != KERN_SUCCESS) {
			panic("BEGONIA CPU: cannot create startup thread: %d", result);
		}
		thread_deallocate(thread);
	}
	return true;
}

/* Do not wait in a nub's start(): matching can be asynchronous or serialized. */
void
BegoniaCPU::startAll(void *arg __unused, wait_result_t result __unused)
{
	const ml_topology_info_t *topology = ml_get_topology_info();
	uint32_t requested = 1;
	PE_parse_boot_argn("cpus", &requested, sizeof(requested));
	IOLog("BEGONIA CPU: requested=%u selected=%u (limit=%u)\n",
	    requested, topology->num_cpus, BEGONIA_MAX_CPUS);

	uint64_t deadline;
	nanoseconds_to_deadline(10 * NSEC_PER_SEC, &deadline);
	while (__atomic_load_n(&begonia_cpus_registered, __ATOMIC_ACQUIRE) != topology->num_cpus) {
		if (mach_absolute_time() > deadline) {
			for (unsigned int i = 0; i < topology->num_cpus; i++) {
				if (__atomic_load_n(&begonia_cpus[i], __ATOMIC_ACQUIRE) == NULL) {
					IOLog("BEGONIA CPU: cpu=%u NOT registered (IOKit matching timeout)\n", i);
				}
			}
			panic("BEGONIA CPU: CPU registration timeout");
		}
		IOSleep(1);
	}
	/* The supplied ADT has cpu0 as the boot CPU and ascending nodes. */
	if (topology->boot_cpu->cpu_id != 0 || topology->boot_cpu->phys_id != 0) {
		panic("BEGONIA CPU: expected boot MPIDR 0 and logical CPU 0");
	}
	for (unsigned int i = 0; i < topology->num_cpus; i++) {
		BegoniaCPU *bcpu = __atomic_load_n(&begonia_cpus[i], __ATOMIC_ACQUIRE);
		kern_return_t kr = processor_start(bcpu->machProcessor);
		if (kr != KERN_SUCCESS) {
			panic("BEGONIA CPU: cpu=%u processor_start failed: %d", i, kr);
		}
		/* processor_start() is asynchronous for secondaries in this XNU. */
		nanoseconds_to_deadline(5 * NSEC_PER_SEC, &deadline);
		while (__atomic_load_n(&bcpu->initialized, __ATOMIC_ACQUIRE) == 0 ||
		    !begonia_cpu_signal_is_enabled(i)) {
			if (mach_absolute_time() > deadline) {
				IOLog("BEGONIA CPU: cpu=%u mpidr=0x%x NOT alive after 5s\n",
				    i, topology->cpus[i].phys_id);
				panic("BEGONIA CPU: cpu=%u startup timeout (reset/MMU/GIC/SGI)", i);
			}
			IOSleep(1);
		}
		IOLog("BEGONIA CPU: cpu=%u mpidr=0x%x came alive; SGI1 OK\n",
		    i, topology->cpus[i].phys_id);
	}
	/* Publish exactly once, after every selected CPU has completed initCPU(). */
	ml_cpu_init_completed();
	__atomic_load_n(&begonia_cpus[topology->boot_cpu->cpu_id], __ATOMIC_ACQUIRE)->cpusRunning();
	IOService::publishResource(gIOAllCPUInitializedKey, kOSBooleanTrue);
	ml_set_max_cpus(topology->max_cpu_id + 1);
	IOLog("BEGONIA CPU: %u/%u CPUs initialized, SGI1 verified\n",
	    topology->num_cpus, topology->num_cpus);
	thread_terminate(current_thread());
}

void
BegoniaCPU::free(void)
{
	OSSafeReleaseNULL(cpuName);
	IOCPU::free();
}

void
BegoniaCPU::initCPU(bool boot)
{
	if (!boot) {
		panic("BEGONIA CPU: cpu=%u resume is not implemented", getCPUNumber());
	}
	assert(cpu_number() == (int)getCPUNumber());

	ml_broadcast_cpu_event(CPU_BOOTED, getCPUNumber());

	/* Secondary startup runs at splsched: queue the self-IPI and return.
	 * startAll waits for its delivery after the startup thread unmasks IRQs.
	 * Waiting here with DAIF.I set would deadlock every secondary. */
	signalCPU(this);
	setCPUState(kIOCPUStateRunning);
	kprintf("BEGONIA CPU: cpu=%u entered initCPU mpidr=0x%llx EL%llu IRQs=%u\n",
	    getCPUNumber(), __builtin_arm_rsr64("MPIDR_EL1") & 0xffffULL,
	    __builtin_arm_rsr64("CurrentEL") >> 2, ml_get_interrupts_enabled());
	__atomic_store_n(&initialized, 1, __ATOMIC_RELEASE);
}

void
BegoniaCPU::signalCPU(IOCPU *target)
{
	const ml_topology_info_t *topology = ml_get_topology_info();
	unsigned int logical = target->getCPUNumber();
	if (logical >= topology->num_cpus) {
		panic("BEGONIA CPU: invalid SGI target %u", logical);
	}
	/* Use the TARGET affinity, including Aff1, never the sender's MPIDR. */
	uint64_t mpidr = topology->cpus[logical].phys_id;
	unsigned int aff0 = mpidr & 0xff;
	if (aff0 >= 16) {
		panic("BEGONIA CPU: SGI target requires unsupported range selector");
	}
	uint64_t sgi = (((mpidr >> 32) & 0xff) << 48) |
	    (((mpidr >> 16) & 0xff) << 32) |
	    (((mpidr >> 8) & 0xff) << 16) |
	    ((uint64_t)1 << 24) | (1ULL << aff0);
	__builtin_arm_dsb(DSB_ISHST);
	__builtin_arm_wsr64("ICC_SGI1R_EL1", sgi);
	__builtin_arm_isb(ISB_SY);
}

kern_return_t
BegoniaCPU::startCPU(vm_offset_t start_paddr, vm_offset_t arg_paddr)
{
	const ml_topology_cpu_t *cpu = &ml_get_topology_info()->cpus[getCPUNumber()];
	/* cpu_start() passes NULL,NULL: x20/x21 are set by LowResetVectorBase. */
	if (start_paddr != 0 || arg_paddr != 0 || cpu->phys_id == 0 ||
	    __atomic_load_n(&initialized, __ATOMIC_ACQUIRE) != 0) {
		panic("BEGONIA CPU: cpu=%u unsupported restart", getCPUNumber());
	}
	vm_offset_t entry = ml_static_vtop((vm_offset_t)&begonia_secondary_entry);
	IOLog("BEGONIA CPU: cpu=%u CPU_ON target=0x%x entry_pa=0x%llx context=%u\n",
	    getCPUNumber(), cpu->phys_id, (uint64_t)entry, getCPUNumber());
	__builtin_arm_dsb(DSB_SY);
	int32_t rc = begonia_psci_cpu_on(cpu->phys_id, entry, getCPUNumber());
	IOLog("BEGONIA CPU: cpu=%u PSCI CPU_ON rc=%d\n", getCPUNumber(), rc);
	/* cpu_start() discards PE_cpu_start's return; do not silently carry on. */
	if (rc != 0) {
		panic("BEGONIA CPU: cpu=%u NOT alive: PSCI CPU_ON rc=%d", getCPUNumber(), rc);
	}
	return KERN_SUCCESS;
}

void
BegoniaCPU::quiesceCPU(void)
{
	panic("BEGONIA CPU: CPU quiesce is not implemented");
}

void
BegoniaCPU::haltCPU(void)
{
	panic("BEGONIA CPU: CPU halt/suspend is not implemented");
}

const OSSymbol *
BegoniaCPU::getCPUName(void)
{
	return cpuName;
}
#endif /* BEGONIA */
