libsprinkles: Simple disassembly library
========================================
*This project is alpha (which is my excuse if something doesn't
function accurately or correctly).*

libsprinkles is both a library and has an accompanying example called `vorhees`
libsprinkles provides a simple API that can be used to disassemble binaries
using LLVM.  The example program `vorhees` uses libsprinkles and can be used to
disassemble a binary into a JSON representation.

Usage
-----
The best example is to look at vorhees/main.cc which takes as input an object
file and outputs a JSON representation of the executable code.

So you want to use the library?
1. Include "sprinkles.hh"
1. Instantiate a Sprinkles instance providing a path to an object file that is
   to be disassembled: `sprinkles::Sprinkles S(filename);`
1. Initialize the Sprinkles instance (perform the disassembly):
   `S.initialize();`
1. Now you're ready to Sprinkle out some magical assembly!

Building
--------
1. Create a build directory. `mkdir libsprinkles/build`
1. From the just created build directory, invoke cmake with the path to the
   libsprinkles sources. `cd libsprinkles/build; cmake ../`
1. Invoke `make` to build the library and example program `vorhees`.

Dependencies
------------
* [llvm](https://llvm.org/): libsprinkles uses many of the libraries provided
by LLVM.  Build and install LLVM, or use a package (`llvm` and/or `llvm-dev`).

Resources
---------
* https://llvm.org/

Contact
-------
Matt Davis: https://github.com/enferex
