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
	g++ -O3 -Wall -c -fmessage-length=0 -std=c++11 $(shell pkg-config --cflags gtkmm-3.0) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/%.o: ../source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


