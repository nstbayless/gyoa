################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/backend/git/GitOps.cpp 

OBJS += \
./src/backend/git/GitOps.o 

CPP_DEPS += \
./src/backend/git/GitOps.d 


# Each subdirectory must supply rules for building sources it contributes
src/backend/git/%.o: ../src/backend/git/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -std=c++1y -I../includes -I../rapidxml -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


