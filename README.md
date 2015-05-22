
# Getting Started

This project uses cmake to do multi-platform builds. Each platform
will have slightly different instructions.

# Getting Started on Windows

To build this, you'll need Visual Studio 2013 Community (free!) and
CMake (also free!).

```
git clone git@github.com:peachyprinter/c-usb-module.git
cd c-usb-module
mkdir _build
cd _build
cmake .. -G 'Visual Studio 12 2013'
cmake --build . --config Release
```

You'll now have a copy of the DLL built as src/Release/PeachyUSB.dll

You want to make sure to do a Release build instead of a Debug build;
if you do a debug build, people that don't have Visual Studio
installed will not be able to load your DLL.

