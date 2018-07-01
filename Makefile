CC := arm-none-eabi-gcc
CFLAGS := -Wall -Werror -mfloat-abi=soft -march=armv4t -g -c 
INCLUDE_PATH := -I/opt/EmbedSky/4.3.3/armv4t/include -Iport/ -Isrc/ -I../other/driver/inc/ -I../OsPro/ -I../OsPro/UCGUI/Start/GUI/WM/ -I../OsPro/UCGUI/Start/GUI/Widget/ -I../OsPro/UCGUI/Start/GUI/Core/ -I../OsPro/UCGUIStart/JPEG/ -I../OsPro/UCGUI/Start/Config/
OBJS_PATH := objs/
SRCS_S := port/os_cpu_s.S
OBJS_S := $(patsubst %.S, %.o, $(SRCS_S))
SRCS_C := port/os_cpu_c.c \
src/base/os_bitmap_index.c \
src/base/os_deque.c \
src/base/os_list.c \
src/base/os_map.c \
src/base/os_mem_pool.c \
src/base/os_multimap.c \
src/base/os_multiset.c \
src/base/os_set.c \
src/base/os_string.c \
src/base/os_tree.c \
src/base/os_vector.c \
src/fs/disk_driver.c \
src/fs/os_cluster.c \
src/fs/os_dentry.c \
src/fs/os_file.c \
src/fs/os_fs.c \
src/fs/os_fs_port.c \
src/fs/os_journal.c \
src/mem/os_buddy.c \
src/mem/os_mem.c \
src/sched/os_pid.c \
src/sched/os_q.c \
src/sched/os_sched.c \
src/sched/os_sem.c \
src/sched/os_timer.c \
src/vfs/os_vfs.c \
src/vtos.c
OBJS_C := $(patsubst %.c, %.o, $(SRCS_C))
all : $(OBJS_PATH) $(OBJS_S) $(OBJS_C)

$(OBJS_PATH) :
	@echo "build obj path"
	@mkdir $(OBJS_PATH)

%.o : %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH)
	@cp $@ $(OBJS_PATH)$(notdir $@)
%.o : %.S
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH)
	@cp $@ $(OBJS_PATH)$(notdir $@)
.PHONY : clean
clean :
	@rm -rf $(OBJS_PATH) $(OBJS_S) $(OBJS_C)
