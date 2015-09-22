## Introduction
The IoTivity extensions project implements the IoTivity Web APIs on top of the Crosswalk app runtime and its Extensions framework.

## Build Instructions
Build requires:
* IotTivity
* Crosswalk 
* boost-devel
* python

Build commands:
```
$ make
$ make install DESTDIR=<lib output dir>
```

Example for Tizen build available in `tools/tizen/iotivity-extensions-crosswalk.spec` 

## License
This project's code uses the BSD license, see our `LICENSE` file.
