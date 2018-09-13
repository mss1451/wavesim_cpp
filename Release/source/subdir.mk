################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../source/INIReader.cpp \
../source/main.cpp \
../source/project_handler.cpp \
../source/wave_engine.cpp \
../source/window_handler.cpp 

C_SRCS += \
../source/ini.c \
../source/zutil.c 

OBJS += \
./source/INIReader.o \
./source/ini.o \
./source/main.o \
./source/project_handler.o \
./source/wave_engine.o \
./source/window_handler.o \
./source/zutil.o 

CPP_DEPS += \
./source/INIReader.d \
./source/main.d \
./source/project_handler.d \
./source/wave_engine.d \
./source/window_handler.d 

C_DEPS += \
./source/ini.d \
./source/zutil.d 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/gtkmm-3.0 -I/usr/include/gdkmm-3.0 -I/usr/include/glibmm-2.4 -I/usr/lib/x86_64-linux-gnu/glibmm-2.4/include -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/sigc++-2.0 -I/usr/lib/x86_64-linux-gnu/sigc++-2.0/include -I/usr/lib/x86_64-linux-gnu/gtkmm-3.0/include -I/usr/lib/x86_64-linux-gnu/gdkmm-3.0/include -I/usr/lib/x86_64-linux-gnu/pangomm-1.4/include -I/usr/include/pangomm-1.4 -I/usr/include/pango-1.0 -I/usr/include/cairomm-1.0 -I/usr/include/cairo -I/usr/lib/x86_64-linux-gnu/cairomm-1.0/include -I/usr/include/freetype2 -I/usr/include/atkmm-1.6 -I/usr/include/gtk-3.0 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/giomm-2.4 -I/usr/include/atk-1.0 -I/usr/lib/x86_64-linux-gnu/giomm-2.4/include -O3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/%.o: ../source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


