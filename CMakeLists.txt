cmake_minimum_required (VERSION 3.0.0 FATAL_ERROR)
project (blepp_scan)

include_directories(
  /usr/local/include
)

add_executable (blepp_scan blepp_scan.cpp)
set_target_properties(blepp_scan
    PROPERTIES
    CXX_STANDARD 17)

target_link_libraries (blepp_scan ble++)
