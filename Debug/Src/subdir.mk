################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/codegen.c \
../Src/dapt_params.c \
../Src/loop_scop.c \
../Src/loop_tile.c \
../Src/main.c \
../Src/norm_map.c \
../Src/norm_scop.c \
../Src/tool_debug.c \
../Src/tool_isl.c \
../Src/tool_string.c 

OBJS += \
./Src/codegen.o \
./Src/dapt_params.o \
./Src/loop_scop.o \
./Src/loop_tile.o \
./Src/main.o \
./Src/norm_map.o \
./Src/norm_scop.o \
./Src/tool_debug.o \
./Src/tool_isl.o \
./Src/tool_string.o 

C_DEPS += \
./Src/codegen.d \
./Src/dapt_params.d \
./Src/loop_scop.d \
./Src/loop_tile.d \
./Src/main.d \
./Src/norm_map.d \
./Src/norm_scop.d \
./Src/tool_debug.d \
./Src/tool_isl.d \
./Src/tool_string.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o: ../Src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


