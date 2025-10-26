RM := rm -rf

LIBS := -lpet -lisl
OBJ_SRCS :=
C_DEPS :=

-include debug/obj/makefile.mk
	
# All Target
all: debug/dapt

# Tool invocations
debug/dapt: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	g++ -L/usr/local/lib -o "debug/dapt" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) "debug/dapt" $(OBJS) $(C_DEPS)
	-@echo ' '

.PHONY: all clean dependents