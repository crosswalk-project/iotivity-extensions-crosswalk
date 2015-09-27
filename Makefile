# IoTivity params
IOTIVITY_VERSION ?= 0.9.2
IOTIVITY_REBUILD ?=
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
	@echo "IOTIVITY_INC_PATH = $(IOTIVITY_INC_PATH)"
	@echo "IOTIVITY_LIB_PATH = $(IOTIVITY_LIB_PATH)"
	$(CC) $(EXTENSION_CCFLAGS) -fpermissive -shared -o build/libiotivity-extension.so \
        $(SYSROOT_FLAGS) \
	-I./ \
	$(IOTIVITY_INC_PATH) \
	$(IOTIVITY_LIB_PATH) \
	$(SOURCES)

prepare:
	mkdir -p $(BUILD_DIR) 

build_iotivity: prepare
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

.PHONY: all build_iotivity clean install prepare
