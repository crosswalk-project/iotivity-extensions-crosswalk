## Introduction
The IoTivity extensions project implements the IoTivity Web APIs on top of the Crosswalk app runtime and its Extensions framework.

## Build Instructions
Build requires:
* IoTivity
* Python-2.7

Build commands:
To rebuild IoTivity with the extension:
```
$ IOTIVITY_REBUILD=true make
$ make install DESTDIR=<lib output dir>
```

Or, if IoTvity is already available, just do :
```
$ make
$ make install DESTDIR=<lib output dir>
```

For details:
```
$ make help
```

Example for Tizen build available in `tools/tizen/iotivity-extensions-crosswalk.spec` 

## License
This project's code uses the BSD license, see our `LICENSE` file.
