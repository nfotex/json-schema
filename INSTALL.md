json-schema uses [cmake](https://cmake.org) to build.

For running the tests, you need to have [perl](https://www.perl.org).

You'll need [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
compiled as a shared library (`BUILD_SHARED_LIBS=ON`) and
[pcre](http://www.pcre.org/).

The basic usage is
```sh
mkdir build
cd build
cmake ..
make
make test
make install
```

Some useful parameters you can pass to `cmake` with `-Dparameter=value`:

- `CMAKE_INSTALL_PREFIX`: for setting the installation path

You can get verbose build output with by passing `VERBOSE=1` to `make`.

You can also check the [cmake FAQ](https://cmake.org/Wiki/CMake_FAQ).
