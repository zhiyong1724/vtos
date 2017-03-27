CC := arm-linux-gcc-4.3.2
LD := arm-linux-ld
CP := arm-linux-objcopy
LIBS_PATH := -L/usr/local/arm/4.3.2/lib/gcc/arm-none-linux-gnueabi/4.3.2/armv4t -lgcc
INCLUDE_PATH := -Iapp -Idrive -Ivtos/port -Ivtos/src
CFLAGS := -Wall -Werror -mfloat-abi=soft -march=armv4 -g -c
ELF := vtos.elf
TARGET := vtos.bin
OBJS_PATH := obj/
APP_PATH := app/
DRIVE_PATH := drive/
ARM_LIB_PATH := arm_lib/
PORT_PATH := vtos/port/
SRC_PATH := vtos/src/
BASE_PATH := vtos/src/base/
LIB_PATH := vtos/src/lib/
MEM_PATH := vtos/src/mem/
SCHED_PATH := vtos/src/sched/
SRCS_S := start_s.S \
          os_cpu_s.S 
OBJS_S := $(patsubst %.S, $(OBJS_PATH)%.o, $(SRCS_S))
SRCS_C := start_c.c \
          nandflash.c \
          led.c \
          uart.c \
          timer.c \
          main.c \
          os_cpu_c.c \
          vtos.c \
          os_mem.c \
          os_mem_pool.c \
          os_list.c \
          os_pid.c \
          os_sem.c \
          os_q.c \
          os_sched.c \
          os_timer.c \
          os_tree.c \
          os_string.c 
OBJS_C := $(patsubst %.c, $(OBJS_PATH)%.o, $(SRCS_C))
all : $(OBJS_PATH) $(OBJS_PATH)$(ELF)
	@echo "CP $(TARGET)"
	@$(CP) -O binary $(OBJS_PATH)$(ELF) $(OBJS_PATH)$(TARGET)
$(OBJS_PATH) :
	@echo "build obj path"
	@mkdir $(OBJS_PATH)
$(OBJS_PATH)$(ELF) : $(OBJS_S) $(OBJS_C)
	@echo "LD $(ELF)"
	@$(LD) -Tvtos.lds $^ -o $(OBJS_PATH)$(ELF) $(INCLUDE_PATH) $(LIBS_PATH)
$(OBJS_PATH)%.o : $(APP_PATH)%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH)
$(OBJS_PATH)%.o : $(DRIVE_PATH)%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH)
$(OBJS_PATH)%.o : $(PORT_PATH)%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH)
$(OBJS_PATH)%.o : $(SRC_PATH)%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH)
$(OBJS_PATH)%.o : $(BASE_PATH)%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH)
$(OBJS_PATH)%.o : $(LIB_PATH)%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH)
$(OBJS_PATH)%.o : $(MEM_PATH)%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH)
$(OBJS_PATH)%.o : $(SCHED_PATH)%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH)
$(OBJS_PATH)%.o : $(ARM_LIB_PATH)%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH)
$(OBJS_PATH)%.o : $(ARM_LIB_PATH)%.S
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH)
$(OBJS_PATH)%.o : $(PORT_PATH)%.S
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH)
.PHONY : clean
clean :
	@rm $(OBJS_PATH)*.elf $(OBJS_PATH)*.o $(OBJS_PATH)*.bin
