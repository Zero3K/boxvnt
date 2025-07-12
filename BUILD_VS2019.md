# Building with Visual Studio 2019

This project can now be built with Visual Studio 2019 in addition to the original Open Watcom C/C++ compiler.

## Prerequisites

1. **Visual Studio 2019** with C++ development tools
2. **Windows Driver Kit (WDK) for Windows 10** - This is required for building kernel drivers
   - Download from: https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
   - The WDK includes the necessary headers, libraries, and build tools for kernel development

## Building

1. Open `boxvideo.sln` in Visual Studio 2019
2. Select the desired configuration (Debug or Release)
3. Build the solution (Build -> Build Solution or Ctrl+Shift+B)

The output will be `boxvideo.sys` in the Debug or Release directory.

## Build Configurations

- **Debug**: Includes debug information and symbols for debugging
- **Release**: Optimized build for production use

Both configurations target Windows XP and later (x86 32-bit).

## Notes

- The project is configured as a Windows Driver Model (WDM) kernel driver
- It uses the `videoprt.lib` library from the Windows Driver Kit
- The entry point is set to `DriverEntry` as required for Windows kernel drivers
- All original functionality from the Open Watcom build is preserved

## Compatibility

The code has been updated for compatibility between Open Watcom and MSVC compilers:
- Added `#define inline __inline` for MSVC compatibility
- All other code remains unchanged and should build without issues

The resulting driver is functionally identical to the one built with Open Watcom.