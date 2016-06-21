KERNELDIR = /usr/src/linux-2.4.18-14custom

include $(KERNELDIR)/.config

CFLAGS = -D__KERNEL__ -DMODULE -I$(KERNELDIR)/include -O -Wall

all: mymodule.o

mymodule.o: mix.o sha1.o srandom.o
	$(LD) -r $^ -o $@
