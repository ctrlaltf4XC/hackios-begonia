/*
 * Minimal device-tree platform expert for Xiaomi Redmi Note 8 Pro (begonia).
 * Uses XNU's normal platform, power-domain and nub initialization. Hardware
 * interrupt and power-management services are supplied by separate drivers.
 *
 * The ADT published by the begonia boot shim has compatible "xiaomi,begonia"
 * at the root, so IONameMatch = 'xiaomi,begonia' matches it.
 *
 * All hardware values are read from the ADT at runtime by the GIC/timer
 * drivers; this file only matches the board and publishes CPU/topology nubs.
 */
#include <pexpert/arm64/board_config.h>

#if defined(BEGONIA)
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IOLib.h>

class BegoniaPlatformExpert : public IODTPlatformExpert {
	OSDeclareDefaultStructors(BegoniaPlatformExpert);

public:
	bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
	void processTopLevel(IORegistryEntry *root) APPLE_KEXT_OVERRIDE;
	const char *deleteList(void) APPLE_KEXT_OVERRIDE;
	const char *excludeList(void) APPLE_KEXT_OVERRIDE;
	bool getTargetName(char *name, int maxLength) APPLE_KEXT_OVERRIDE;
	bool getProductName(char *name, int maxLength) APPLE_KEXT_OVERRIDE;
};

OSDefineMetaClassAndStructors(BegoniaPlatformExpert, IODTPlatformExpert);

bool
BegoniaPlatformExpert::start(IOService *provider)
{
	IOLog("BEGONIA: starting device-tree platform expert\n");
	if (!IODTPlatformExpert::start(provider)) {
		return false;
	}
	registerService();
	IOLog("BEGONIA: platform expert registered\n");
	return true;
}

const char *
BegoniaPlatformExpert::deleteList(void)
{
	return "()";
}

const char *
BegoniaPlatformExpert::excludeList(void)
{
	return "('chosen', 'defaults', 'cpus', 'options')";
}

void
BegoniaPlatformExpert::processTopLevel(IORegistryEntry *root)
{
	/*
	 * The stock implementation requires an Apple NVRAM /options node.
	 * BEGONIA has no NVRAM controller; publish real CPU/device nodes without
	 * inventing one. createNubs adds the topology's logical CPU/cluster IDs.
	 */
	IORegistryEntry *cpus = root->childFromPath("cpus", gIODTPlane);
	if (cpus != NULL) {
		createNubs(this, IODTFindMatchingEntries(cpus, kIODTExclusive, NULL));
		cpus->release();
	}
	createNubs(this, IODTFindMatchingEntries(root, kIODTExclusive, excludeList()));
}

bool
BegoniaPlatformExpert::getTargetName(char *name, int maxLength)
{
	if (name == NULL || maxLength <= 0) {
		return false;
	}
	strlcpy(name, "begonia", (size_t)maxLength);
	return true;
}

bool
BegoniaPlatformExpert::getProductName(char *name, int maxLength)
{
	if (name == NULL || maxLength <= 0) {
		return false;
	}
	strlcpy(name, "Xiaomi Redmi Note 8 Pro", (size_t)maxLength);
	return true;
}
#endif /* BEGONIA */
