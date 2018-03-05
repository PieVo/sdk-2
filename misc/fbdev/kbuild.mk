MSTAR_REMOVE_INCS:=all-exception

M?=$(CURDIR)
include $(M)/../../../../project/configs/current.configs
KDIR?=$(PROJ_ROOT)/kbuild/$(KERNEL_VERSION)

EXTRA_CFLAGS      += $(patsubst %,-I%,$(INCS))
$(notdir $(M))-objs += $(patsubst %.c,%.o,$(SRCS))

EXTRA_CFLAGS += -I$(PROJ_ROOT)/../sdk/mhal/include/utopia -include utopia_macros.h
MSTAR_INC_DIR+= drivers/mstar/include
EXTRA_CFLAGS += $(foreach n,$(MSTAR_INC_DIR),-I$(n))
obj-m += $(notdir $(M)).o

module:
	$(MAKE) -C $(KDIR) M=$(M) BASEDIR=$(BASEDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(M) BASEDIR=$(BASEDIR) clean

install:
	cp -f $(notdir $(M)).ko $(PROJ_ROOT)/release/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/lib/modules/$(KERNEL_VERSION)
