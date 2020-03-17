#!/bin/bash
# Prerequisites: cmake and libble++ (https://github.com/edrosten/libblepp)

if [ ! -d build ]; then
    mkdir build
fi

cd build
cmake ..
make
cd -

# Grant permissions required for BLE scanning to the binary file:
BIN_FILE=`realpath ./build/blepp_scan`
sudo setcap cap_net_raw+eip $(eval readlink -f "$BIN_FILE")
