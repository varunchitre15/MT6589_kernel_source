obj-y += mt_etm.o
mt_etm-y += \
	src/mt_etm.o \
	src/utils.o
ccflags-y += -I$(src)/include -D__ETM -D__MT_PREEMPT -D__LINUX_3_0

# HEADER FILES
EXTRA_CFLAGS += -I$(src) -I$(src)/include
# add -D__PARSING for decoding
