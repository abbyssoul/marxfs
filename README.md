# marxFS [![C++ standard][c++-standard-shield]][c++-standard-link] [![License][license-shield]][license-link]
---
[![TravisCI][travis-shield]][travis-link]
[![LGTM][LGTM-shield]][LGTM-link]
[![issues][badge.issues]][issues]

[license-shield]: https://img.shields.io/badge/License-Apache%202.0-blue.svg
[c++-standard-shield]: https://img.shields.io/badge/c%2B%2B-17%2F20-blue
[LGTM-shield]: https://img.shields.io/lgtm/grade/cpp/github/abbyssoul/marxfs.svg
[travis-shield]: https://travis-ci.org/abbyssoul/marxfs.png?branch=master

[c++-standard-link]: https://en.wikipedia.org/wiki/C%2B%2B#Standardization
[license-link]: https://opensource.org/licenses/Apache-2.0
[travis-link]: https://travis-ci.org/abbyssoul/marxfs

[LGTM-link]: https://lgtm.com/projects/g/abbyssoul/marxfs/alerts/

[badge.issues]: https://img.shields.io/github/issues/abbyssoul/marxfs.svg
[issues]: http://github.com/abbyssoul/marxfs/issues


An example of async [9P](https://en.wikipedia.org/wiki/9P_(protocol)) server using [libapsio][libapsio]: serving archives as a 9p filesystem.


# Usage
Once you built the project you can start serving archive files:
```shell
marxfs -H tcp:127.0.0.1:1564 my-file.tag.gz
```

This command starts `marxfs` listening for `tcp` connections on local host port 1564 and serving `my-file.tag.gz` file.
To test the server you can use 9p client of your choice or - if your system supports 9pfs - you can `mount` new fs directly:

```shell
sudo mount -t 9p 127.0.0.1 <directory> \
    -o version=9p2000,port=1564,dfltuid=$(id -u),dfltgid=$(id -g)
```

This will mount archive file to a <directory>. You can then use normal `cd`, `ls`, `cat` or a file browser of your choice to explore
the structure of the archive file as filesystem.


## Slightly more advance usage
[libapsio][libapsio] can listen on multiple addresses and protocols at the same time.
```shell
marxfs -H tcp:127.0.0.1:1564,tcp:0.0.0.0:5164 -H unix:/var/run/marxfs.socket my-file.tar.gz
```

It is also possible to server multiple files at the same time.
```shell
marxfs -H tcp:127.0.0.1:1564 my-data.zip other-file.tar.gz data-file.rar
```

Keep in mind that this is only a limited example though. Contributions are welcomed.

# Building
The project requires C++ compiler supporting C++17 and is using `CMake` for build management.
Default configuration requires `ninja` as build system.
`Makefile` is only used for automation of some manual tasks.

```shell
cd <checkout directory>

# In the project check-out directory:
# To build debug version with sanitizer enabled (recommended for development)
./configure --prefix=/user/home/${USER}/bin

# To build the project. Takes care of CMake and conan dependencies
make

# To build and run unit tests:
make test

# To install into <prefix> location
make  install
```


If you need to build debug / instrumented version, it is recommended to use sanitisers
```shell
./configure --prefix=/user/home/${USER}/bin  --enable-debug --enable-sanitizer
```

### Build tools
In order to build this project following tools must be present in the system:
* git (to check out project and it’s external modules, see dependencies section)
* cmake - user for build script generation
* ninja (opional, used by default)
* cppcheck (opional, but recommended for static code analysis, latest version from git is used as part of the 'codecheck' step)
* cpplint (opional, for static code analysis in addition to cppcheck)
* valgrind (opional, for runtime code quality verification)

## Dependencies
Project dependencies are managed using [Conan.io][conan.io] package manager.
Make sure you have conan installed to build the project.

 * [libapsio][libapsio] - asynchronous 9p server.
 * [libclime][libclime] - command line agruments parser.
 * [GTest][gtest] - unit test framework.

## Testing
The project is using [GTest][gtest] for unit tests.
The source code for unit test located in directory [test](test)

Test suit can (and should) be run via:
```shell
make test
```

### Developers/Contributing
Please make sure that static code check step returns no error before raising a pull request
```shell
make codecheck
```

It is also a good idea to run Valgrind on the test suit to make sure that the memory is safe.
Note, sanitizers are not compatible with Valgrind:
```shell
./configure --enable-debug --disable-sanitizer
make clean && make verify
```

## Contributing changes
The framework is work in progress and contributions are very welcomed.
Please see  [`CONTRIBUTING.md`](CONTRIBUTING.md) for details on how to contribute to
this project.

Please note that in order to maintain code quality a set of static code analysis tools is used as part of the build process.
Thus all contributions must be verified by this tools before PR can be accepted.


## License
The library available under Apache License 2.0
Please see [`LICENSE`](LICENSE) for details.


## Authors
Please see [`AUTHORS`](AUTHORS) file for the list of contributors.


[libapsio]: https://github.com/abbyssoul/libapsio
[libclime]: https://github.com/abbyssoul/libclime
[gtest]: https://github.com/google/googletest
[conan.io]: https://conan.io/
