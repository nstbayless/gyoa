################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/backend/model/Read.cpp 

OBJS += \
./src/backend/model/Read.o 

CPP_DEPS += \
./src/backend/model/Read.d 


# Each subdirectory must supply rules for building sources it contributes
src/backend/model/%.o: ../src/backend/model/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -std=c++1y -I../includes -I../rapidxml -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


