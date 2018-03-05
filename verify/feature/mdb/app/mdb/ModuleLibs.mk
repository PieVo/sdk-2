LIBS     += -Wl,-rpath=./
LIBS     += -L$(DB_OUT_PATH)/lib
LIBS     += -L$(PROJ_ROOT)/release/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/lib/static
LIBS     += -l$(DB_LIB_NAME) -lmi_rgn -lmi_common -lmi_sys