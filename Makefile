IOTIVITY_CCFLAGS=$(CFLAGS) -loc -loc_logger -loctbstack -fPIC -Wall -std=c++11
SRCDIR=iotivity
SOURCES=$(wildcard $(SRCDIR)/*.cc) common/extension.cc

all: libiotivity-extension.so

iotivity_api.cc:
	 python tools/generate_api.py $(SRCDIR)/iotivity_api.js kSource_iotivity_api $(SRCDIR)/iotivity_api.cc

libiotivity-extension.so: prepare iotivity_api.cc
	$(CC) $(IOTIVITY_CCFLAGS) -shared -o build/libiotivity-extension.so \
        $(SYSROOT_FLAGS) -I./ $(SOURCES)

prepare:
	mkdir -p build

install:libiotivity-extension.so
	install -D build/libiotivity-extension.so \
        $(DESTDIR)/libiotivity-extension.so
clean:
	rm -Rf build
	rm extension/coap_api.cc

.PHONY: all prepare clean
