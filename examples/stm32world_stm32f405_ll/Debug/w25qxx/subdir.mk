################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../w25qxx/w25qxx.c 

OBJS += \
./w25qxx/w25qxx.o 

C_DEPS += \
./w25qxx/w25qxx.d 


# Each subdirectory must supply rules for building sources it contributes
w25qxx/%.o w25qxx/%.su w25qxx/%.cyclo: ../w25qxx/%.c w25qxx/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"/home/lth/src/stm32-w25qxx/examples/stm32world_stm32f405_ll/w25qxx" -Og -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-w25qxx

clean-w25qxx:
	-$(RM) ./w25qxx/w25qxx.cyclo ./w25qxx/w25qxx.d ./w25qxx/w25qxx.o ./w25qxx/w25qxx.su

.PHONY: clean-w25qxx

