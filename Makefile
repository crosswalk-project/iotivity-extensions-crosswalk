# IoTivity params
IOTIVITY_VERSION ?= 0.9.2
## Set tu tue if need to rebuilt IoTivity
IOTIVITY_REBUILD ?=
## Scons params
IOTIVITY_BUILD_FLAGS ?= "RELEASE=true"

BUILD_DIR ?= build
IOTIVITY_BUILD_DIR = $(BUILD_DIR)/iotivity-$(IOTIVITY_VERSION)
IOTIVITY_INC_PATH ?= 
IOTIVITY_LIB_PATH ?= 

#IoTivity extensions params
EXTENSION_CCFLAGS = $(CFLAGS) -loc -loc_logger -loctbstack -fPIC -Wall -std=c++11
EXTENSION_SRCDIR = iotivity
SOURCES = $(wildcard $(EXTENSION_SRCDIR)/*.cc) common/extension.cc

ifeq ($(IOTIVITY_REBUILD), true)
REBUILD = build_iotivity 
IOTIVITY_INC_PATH += -I$(IOTIVITY_BUILD_DIR)/resource/csdk/stack/include/ \
        -I$(IOTIVITY_BUILD_DIR)/resource/oc_logger/include \
        -I$(IOTIVITY_BUILD_DIR)/resource/include
IOTIVITY_LIB_PATH += -L$(shell find . -type f -name liboc.so -print -quit | xargs dirname)
endif

all: $(REBUILD) libiotivity-extension.so

iotivity_api.cc:
	 python tools/generate_api.py $(EXTENSION_SRCDIR)/iotivity_api.js kSource_iotivity_api $(EXTENSION_SRCDIR)/iotivity_api.cc

libiotivity-extension.so: prepare iotivity_api.cc
	@echo ''
	@echo "IOTIVITY_INC_PATH = $(IOTIVITY_INC_PATH)"
	@echo "IOTIVITY_LIB_PATH = $(IOTIVITY_LIB_PATH)"
	@echo ''
	$(CC) $(EXTENSION_CCFLAGS) -fpermissive -shared -o build/libiotivity-extension.so \
        $(SYSROOT_FLAGS) \
	-I./ \
	$(IOTIVITY_INC_PATH) \
	$(IOTIVITY_LIB_PATH) \
	$(SOURCES)

prepare:
	mkdir -p $(BUILD_DIR) 

build_iotivity: prepare
	@echo ''
	@echo "***** Rebuilding IoTivity-$(IOTIVITY_VERSION) *****"
	@echo ''
	wget https://github.com/iotivity/iotivity/archive/$(IOTIVITY_VERSION).tar.gz -O $(BUILD_DIR)/iotivity-$(IOTIVITY_VERSION).tar.gz || exit 1;
	tar xvzf  $(BUILD_DIR)/iotivity-$(IOTIVITY_VERSION).tar.gz -C $(BUILD_DIR); \
	cd $(IOTIVITY_BUILD_DIR); \
	git clone https://github.com/01org/tinycbor.git extlibs/tinycbor/tinycbor; \
	scons $(IOTIVITY_BUILD_FLAGS) || exit 1;

install:libiotivity-extension.so
	install -D build/libiotivity-extension.so \
        $(DESTDIR)/libiotivity-extension.so

clean:
	rm -Rf build
	rm -f iotivity/iotivity_api.cc

help:
	@echo ''
	@echo "This Makefile lets you build the Iotivity extensions for Crosswalk on Tizen/Linux."
	@echo "To build:"
	@echo '$$ make'
	@echo '$$ make install DESTDIR=<lib output dir>'
	@echo ''
	@echo "Make Flags:"
	@echo '* IOTIVITY_REBUILD: true to rebuild IoTivity before building the extension'
	@echo '  Default: false'
	@echo '  e.g: $$ IOTIVITY_REBUILD=true make'
	@echo ''
	@echo "* IOTIVITY_VERSION: the version of IoTivity to rebuild. (ingored if IOTIVITY_REBUILD not set)"
	@echo '  Default: 0.9.2'
	@echo '  e.g, to rebuild using IoTivity-0.9.2:'
	@echo '  $$ IOTIVITY_REBUILD=true IOTIVITY_VERSION=0.9.2 make'
	@echo ''
	@echo "* IOTIVITY_BUILD_FLAGS: the scons flags to use to rebuild IoTivity. (ingored if IOTIVITY_REBUILD not set)"
	@echo '  Default: RELEASE=true'
	@echo '  e.g, to rebuild using IoTivity-0.9.2 in DEBUG:'
	@echo '  $$ IOTIVITY_REBUILD=true IOTIVITY_VERSION=0.9.2 IOTIVITY_BUILD_FLAGS="RELEASE=false" make'
	@echo ''
	@echo "* IOTIVITY_INC_PATH: the path to IoTivity headers"
	@echo '  Default: empty, default inc search path used'
	@echo ''
	@echo "* IOTIVITY_LIB_PATH: the path to IoTivity libs"
	@echo '  Default: empty, default lib search path used'
	@echo ''

