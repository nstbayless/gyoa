################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/backend/ops/Operation.cpp 

OBJS += \
./src/backend/ops/Operation.o 

CPP_DEPS += \
./src/backend/ops/Operation.d 


# Each subdirectory must supply rules for building sources it contributes
src/backend/ops/%.o: ../src/backend/ops/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -std=c++1y -I../includes -I../rapidxml -I../include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


