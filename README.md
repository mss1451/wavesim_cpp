# Wave Simulator with C++
This is an example wave simulation engine written in C++. The engine comes with
a native Linux application which uses [gtkmm] and [cairomm]. A java version is
also available.

[gtkmm]: https://www.gtkmm.org/
[cairomm]: https://www.cairographics.org/cairomm/
[java version]: https://github.com/mss1451/wavesim_java
## Prerequisites
gtk and gtkmm development packages (libgtkmm-3.0-dev).
## Building
Assuming that the working directory is the root of the project.
### Building in a separate folder
```sh
$ mkdir build && cd build
$ cmake .. && make
$ mv wavesim_cpp ..
```
### Building in the same folder
```sh
$ cmake . && make
```
## Running
The program needs the **data** folder in the working directory.

```sh
$ ./wavesim_cpp
```
## Acknowledgments
INI file reader provided from <https://github.com/benhoyt/inih>.
