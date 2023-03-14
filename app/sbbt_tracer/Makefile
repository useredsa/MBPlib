ifndef PIN_ROOT
	$(error PIN_ROOT must point to the location of Intel PIN tools.)
endif

CONFIG_ROOT:=$(PIN_ROOT)/source/tools/Config
include $(CONFIG_ROOT)/makefile.config
include $(TOOLS_ROOT)/Config/makefile.default.rules

all: intel64

intel64:
	mkdir -p obj-intel64
	$(MAKE) TARGET=intel64 obj-intel64/sbbt_tracer.so

clean-all:
	$(MAKE) TARGET=intel64 clean
