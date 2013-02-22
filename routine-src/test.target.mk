# This file is generated by gyp; do not edit.

TOOLSET := target
TARGET := test
DEFS_Default :=

# Flags passed to all source files.
CFLAGS_Default := \
	-fPIC -Wall -g3

# Flags passed to only C files.
CFLAGS_C_Default :=

# Flags passed to only C++ files.
CFLAGS_CC_Default :=

INCS_Default := \
	-Irain-src/include \
	-Iroutine-src/test_routine

OBJS := \
	$(obj).target/$(TARGET)/routine-src/test_routine/test_main.o

# Add to the list of files we specially track dependencies for.
all_deps += $(OBJS)

# CFLAGS et al overrides must be target-local.
# See "Target-specific Variable Values" in the GNU Make manual.
$(OBJS): TOOLSET := $(TOOLSET)
$(OBJS): GYP_CFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE))  $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_C_$(BUILDTYPE))
$(OBJS): GYP_CXXFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE))  $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_CC_$(BUILDTYPE))

# Suffix rules, putting all outputs into $(obj).

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(srcdir)/%.c FORCE_DO_CMD
	@$(call do_cmd,cc,1)

# Try building from generated source, too.

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(obj).$(TOOLSET)/%.c FORCE_DO_CMD
	@$(call do_cmd,cc,1)

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(obj)/%.c FORCE_DO_CMD
	@$(call do_cmd,cc,1)

# End of this set of suffix rules
### Rules for final target.
LDFLAGS_Default := \
	-Wl,-E -pg 

LIBS :=

$(obj).target/routine-src/libtest.so: GYP_LDFLAGS := $(LDFLAGS_$(BUILDTYPE))
$(obj).target/routine-src/libtest.so: LIBS := $(LIBS)
$(obj).target/routine-src/libtest.so: LD_INPUTS := $(OBJS)
$(obj).target/routine-src/libtest.so: TOOLSET := $(TOOLSET)
$(obj).target/routine-src/libtest.so: $(OBJS) FORCE_DO_CMD
	$(call do_cmd,solink)

all_deps += $(obj).target/routine-src/libtest.so
# Add target alias
.PHONY: test
test: $(builddir)/lib.target/libtest.so

# Copy this to the shared library output path.
$(builddir)/lib.target/libtest.so: TOOLSET := $(TOOLSET)
$(builddir)/lib.target/libtest.so: $(obj).target/routine-src/libtest.so FORCE_DO_CMD
	$(call do_cmd,copy)

all_deps += $(builddir)/lib.target/libtest.so
# Short alias for building this shared library.
.PHONY: libtest.so
libtest.so: $(obj).target/routine-src/libtest.so $(builddir)/lib.target/libtest.so

# Add shared library to "all" target.
.PHONY: all
all: $(builddir)/lib.target/libtest.so

