# Wave Simulator with Java and C++
This is an example wave simulation engine written in both Java and C++. The engine comes with a native Linux application which uses [gtkmm](https://www.gtkmm.org/) and [cairomm](https://www.cairographics.org/cairomm/) and an example Java application. For more details on what this project is about, see its [Code Project Article](https://www.codeproject.com/Articles/1259631/Wave-Simulator-with-Java-and-Cplusplus)
## Prerequisites
### Java
This is a NetBeans project. Java 8 or later is required.
### C++
This is an Eclipse CDT project. The following development packages are required.
1. libgtkmm-3.0-dev
3. libglibmm-2.4-dev
4. libcairomm-1.0-dev
5. libpangomm-1.4-dev
6. libatkmm-1.6-dev
7. libgdk-pixbuf2.0-dev

It is also required to install the core packages such as libgtk-3-dev. You need **pkg-config** installed for the compiler to resolve the include and library directories.
## Tested On
### Java
- Windows 10 Home (64-bit)
### C++
- Linux Mint 19 Tara - Cinnamon (64-bit)
- Pardus 17.3 - XFCE (64-bit)
## Building
### C++
In a terminal, enter either the **Release** or **Debug** directories depending on which version of the program you want to build. Let's build a **Release** version.

Enter the **Release** directory by entering the following command.

```cd {path_to_project}/Release```

Clean before building.

```make clean```

Build now.

```make```

After a successful build, there should be the executable with the name **wavesim_cpp**.
## Running
There are 64-bit and 32-bit executables inside the **binary_*** directories which can be run directly. Move one of them to the upper directory where the **data** folder exists and run it or, if you have just built from the source, do the following.

After building, move the executable from either the **Release** or **Debug** directory to the upper directory where the **data** folder exists. The program simply needs the **data** on its working directory so you can also move the folder to the executable directory instead. Let's move the exectuable from **Release** to the upper directory.

Enter the **Release** directory.

```cd {path_to_project}/Release```

Move the executable to the upper directory.

```mv ./wavesim_cpp ..```

Enter the upper directory.

```cd ..```

Run the program.

```./wavesim_cpp```
## Authors
Mustafa Sami Salt
## Acknowledgments
INI file reader provided from https://github.com/benhoyt/inih.
