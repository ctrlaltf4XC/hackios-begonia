# makedefs/MakeInc.def changes — BEGONIA machine config

Add BEGONIA to the supported ARM64 machine configs and define its flags.
Modeled on ios-sweet `0004-sweet-machine-config.patch` MakeInc.def changes.

```diff
@@ -29,7 +29,7 @@ SUPPORTED_ARM64_MACHINE_CONFIGS = NONE
 else ifneq ($(findstring _host,$(RC_ProjectName)),)
 SUPPORTED_ARM64_MACHINE_CONFIGS = NONE
 else
-SUPPORTED_ARM64_MACHINE_CONFIGS = BCM2837 T6000 T8101 T8103 VMAPPLE
+SUPPORTED_ARM64_MACHINE_CONFIGS = BCM2837 T6000 T8101 T8103 VMAPPLE BEGONIA
 
 endif
 
@@ -61,6 +61,7 @@ MACHINE_FLAGS_ARM64_T8103 = -DARM64_BOARD_CONFIG_T8103                  -mcpu=ap
 MACHINE_FLAGS_ARM64_T6000 = -DARM64_BOARD_CONFIG_T6000 		-mcpu=apple-a14
 MACHINE_FLAGS_ARM64_VMAPPLE = -DARM64_BOARD_CONFIG_VMAPPLE              -march=armv8.5a
 MACHINE_FLAGS_ARM64_BCM2837 = -DARM64_BOARD_CONFIG_BCM2837
+MACHINE_FLAGS_ARM64_BEGONIA = -DARM64_BOARD_CONFIG_BEGONIA             -march=armv8.2-a+crypto
 
 #
@@ -259,8 +260,11 @@ ifeq ($(ARCH_STRING_FOR_CURRENT_MACHINE_CONFIG),)
 # Without embdedded device map, use a default arch string
 export ARCH_STRING_FOR_CURRENT_MACHINE_CONFIG := $(shell echo $(CURRENT_ARCH_CONFIG) | tr A-Z a-z)
 ifneq ($(filter arm64,$(ARCH_STRING_FOR_CURRENT_MACHINE_CONFIG)),)
+# begonia's Cortex-A76/A55 have no pointer authentication: stay plain arm64
+ifneq ($(CURRENT_MACHINE_CONFIG),BEGONIA)
 export ARCH_STRING_FOR_CURRENT_MACHINE_CONFIG := arm64e
 endif
+endif
 
 endif
```

Also add `proc_reg.h` cache-line definitions for the generic ARM core (modeled on sweet's `elif defined(SWEET)` block):

```diff
@@ -213,6 +213,14 @@
 /* D-Cache. */
 #define MMU_CLINE   6
 
+#elif defined (BEGONIA) /* MediaTek MT6785: Cortex-A76 + Cortex-A55 */
+
+/* I-Cache. */
+#define MMU_I_CLINE 6
+
+/* D-Cache. */
+#define MMU_CLINE   6
+
 #elif defined (VMAPPLE)
```
