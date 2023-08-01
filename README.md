# Shihai's SRS-VMM Data Reader

## CMakeLists.txt

This project uses PcapPlusPlus, which is a multiplatform C++ library for capturing, parsing and crafting of network packets. Please specify the path of PcapPlusPlus in CMakeLists.txt.

This is an example of PcapPlusPlus path on macOS (Homebrew):
```cmake
set(PCAPPP_INCLUDE_DIR /opt/homebrew/Cellar/pcapplusplus/22.11/include/pcapplusplus)
```