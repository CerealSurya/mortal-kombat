################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
ticlang/%.o: ../ticlang/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang" -c -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"/Users/cerealsurya/Desktop/mortal-kombat/ECE319K_Lab9H" -I"/Users/cerealsurya/Desktop/mortal-kombat/ECE319K_Lab9H/Debug" -I"/Users/cerealsurya/ti/mspm0_sdk_2_05_00_05/source/third_party/CMSIS/Core/Include" -I"/Users/cerealsurya/ti/mspm0_sdk_2_05_00_05/source" -D__MSPM0G3507__ -gdwarf-3 -MMD -MP -MF"ticlang/$(basename $(<F)).d_raw" -MT"$(@)" -I"/Users/cerealsurya/Desktop/mortal-kombat/ECE319K_Lab9H/Debug/syscfg"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


