################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../littlefs/runners/bench_runner.c \
../littlefs/runners/test_runner.c 

OBJS += \
./littlefs/runners/bench_runner.o \
./littlefs/runners/test_runner.o 

C_DEPS += \
./littlefs/runners/bench_runner.d \
./littlefs/runners/test_runner.d 


# Each subdirectory must supply rules for building sources it contributes
littlefs/runners/%.o littlefs/runners/%.su littlefs/runners/%.cyclo: ../littlefs/runners/%.c littlefs/runners/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"/home/lth/src/stm32-w25qxx/examples/sl_mcu_stm32f407_littlefs/w25qxx" -I"/home/lth/src/stm32-w25qxx/examples/sl_mcu_stm32f407_littlefs/littlefs" -Og -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-littlefs-2f-runners

clean-littlefs-2f-runners:
	-$(RM) ./littlefs/runners/bench_runner.cyclo ./littlefs/runners/bench_runner.d ./littlefs/runners/bench_runner.o ./littlefs/runners/bench_runner.su ./littlefs/runners/test_runner.cyclo ./littlefs/runners/test_runner.d ./littlefs/runners/test_runner.o ./littlefs/runners/test_runner.su

.PHONY: clean-littlefs-2f-runners

