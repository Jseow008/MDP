################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (11.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../PeripheralDriver/Src/gyro.c \
../PeripheralDriver/Src/i2c.c \
../PeripheralDriver/Src/oled.c 

OBJS += \
./PeripheralDriver/Src/gyro.o \
./PeripheralDriver/Src/i2c.o \
./PeripheralDriver/Src/oled.o 

C_DEPS += \
./PeripheralDriver/Src/gyro.d \
./PeripheralDriver/Src/i2c.d \
./PeripheralDriver/Src/oled.d 


# Each subdirectory must supply rules for building sources it contributes
PeripheralDriver/Src/%.o PeripheralDriver/Src/%.su PeripheralDriver/Src/%.cyclo: ../PeripheralDriver/Src/%.c PeripheralDriver/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I"C:/Users/Javier/Desktop/MDP(1)/MDP/PeripheralDriver/Inc" -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-PeripheralDriver-2f-Src

clean-PeripheralDriver-2f-Src:
	-$(RM) ./PeripheralDriver/Src/gyro.cyclo ./PeripheralDriver/Src/gyro.d ./PeripheralDriver/Src/gyro.o ./PeripheralDriver/Src/gyro.su ./PeripheralDriver/Src/i2c.cyclo ./PeripheralDriver/Src/i2c.d ./PeripheralDriver/Src/i2c.o ./PeripheralDriver/Src/i2c.su ./PeripheralDriver/Src/oled.cyclo ./PeripheralDriver/Src/oled.d ./PeripheralDriver/Src/oled.o ./PeripheralDriver/Src/oled.su

.PHONY: clean-PeripheralDriver-2f-Src

