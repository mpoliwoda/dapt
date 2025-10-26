OBJS += $(patsubst %.c,debug/obj/%.o,$(wildcard *.c))
C_DEPS += $(patsubst %.c,debug/obj/%.d,$(wildcard *.c))

debug/obj/%.o: ./%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '