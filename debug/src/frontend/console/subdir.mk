################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/frontend/console/ConsoleUI.cpp \
../src/frontend/console/ConsoleUI_git.cpp \
../src/frontend/console/ConsoleUI_io.cpp 

OBJS += \
./src/frontend/console/ConsoleUI.o \
./src/frontend/console/ConsoleUI_git.o \
./src/frontend/console/ConsoleUI_io.o 

CPP_DEPS += \
./src/frontend/console/ConsoleUI.d \
./src/frontend/console/ConsoleUI_git.d \
./src/frontend/console/ConsoleUI_io.d 


# Each subdirectory must supply rules for building sources it contributes
src/frontend/console/%.o: ../src/frontend/console/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -std=c++1y -I../includes -I../rapidxml -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


