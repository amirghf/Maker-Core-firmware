################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
K:/Projects/1405/01-Multi_Works_Machine/Codes/Printer\ 3D/My_Framware/Maker\ Core\ firmware/ui/ui_home.cpp 

OBJS += \
./ui/ui_home.o 

CPP_DEPS += \
./ui/ui_home.d 


# Each subdirectory must supply rules for building sources it contributes
ui/ui_home.o: K:/Projects/1405/01-Multi_Works_Machine/Codes/Printer\ 3D/My_Framware/Maker\ Core\ firmware/ui/ui_home.cpp ui/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=c++17 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I"K:/Projects/1405/01-Multi_Works_Machine/Codes/Printer 3D/My_Framware/Maker Core firmware/boards/mw_x1/cubemx/../../../hal" -I"K:/Projects/1405/01-Multi_Works_Machine/Codes/Printer 3D/My_Framware/Maker Core firmware/boards/mw_x1/cubemx/hal" -I"K:/Projects/1405/01-Multi_Works_Machine/Codes/Printer 3D/My_Framware/Maker Core firmware/boards/mw_x1/cubemx/.." -I../FATFS/Target -I../FATFS/App -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../USB_HOST/App -I../USB_HOST/Target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"K:/Projects/1405/01-Multi_Works_Machine/Codes/Printer 3D/My_Framware/Maker Core firmware/core" -I"K:/Projects/1405/01-Multi_Works_Machine/Codes/Printer 3D/My_Framware/Maker Core firmware/boards/mw_x1/cubemx/../../../gcode" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fno-exceptions -fno-rtti -fno-threadsafe-statics -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-ui

clean-ui:
	-$(RM) ./ui/ui_home.cyclo ./ui/ui_home.d ./ui/ui_home.o ./ui/ui_home.su

.PHONY: clean-ui

