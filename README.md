# Wave Simulator with C++
This is an example wave simulation engine written in C++. The engine comes with a native Linux application which uses [gtkmm](https://www.gtkmm.org/) and [cairomm](https://www.cairographics.org/cairomm/). The java version is available [here](https://github.com/mss1451/wavesim_java).
## Prerequisites
gtk and gtkmm development packages (libgtkmm-3.0-dev).
## Building
In a terminal, enter either the **Release** or **Debug** directories depending on which version of the program you want to build. Let's build a **Release** version.

Enter the **Release** directory:
```sh
$ cd {path_to_project}/Release
```
Clean before building:
```sh
$ make clean
```
Build:
```sh
$ make
```
After a successful build, there should be the executable with the name **wavesim_cpp**.
## Running
After building, move the executable from either the **Release** or **Debug** directory to the upper directory where the **data** folder exists. The program simply needs the **data** on its working directory so you can also move the folder to the executable directory instead. Let's move the exectuable from **Release** to the upper directory.

Enter the **Release** directory:
```sh
$ cd {path_to_project}/Release
```
Move the executable to the upper directory:
```sh
$ mv ./wavesim_cpp ..
```
Enter the upper directory:
```sh
$ cd ..
```
Run the program:
```sh
$ ./wavesim_cpp
```
## Acknowledgments
INI file reader provided from https://github.com/benhoyt/inih.
