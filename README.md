Background
==========

KRO image files were created by authors of the AutoPano tools to independently get around the 30K limitation of many 
other image formats, it is basically a simple image format that stores all raw data in uncompressed format. 
A detailed description can be found on [Paul Bourke's website](https://paulbourke.net/dataformats/kro/), 
a snapshot is provided here as well.

Despite its simplicity, there are few open source libraries that manipulates the format, the libkro is therefore here 
to provide a convenient way for reading and writing KRO format.


License
=======

libkro is covered by BSD open source licenses.
Refer to [LICENSE](LICENSE) for license terms.


Building libkro
===============
if you are working on *nix OS, you can use the following command:
```bash
cmake CMakeLists.txt -DBUILD_EXAMPLES=ON
make
```
if you are working on Windows, before proceeding, you should install [cmake](https://cmake.org/) and MinGW64, 
and build with the following command:
```bash
cmake CMakeLists.txt -DBUILD_EXAMPLES=ON
mingw32-make -f Makefile
```

Using libkro
============
There two examples located in ./example folder, refer to them.
