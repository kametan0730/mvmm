PREFIX = aarch64-linux-gnu-
CC = $(PREFIX)gcc
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy

CPU = cortex-a72
QCPU = cortex-a72

CFLAGS = -Wall -Og -g -MD -ffreestanding -nostdinc -nostdlib -nostartfiles -mcpu=$(CPU)
CFLAGS += -I ./include/
LDFLAGS = -nostdlib -nostartfiles

#QEMUPREFIX = ~/qemu/build/
QEMUPREFIX = 
QEMU = $(QEMUPREFIX)qemu-system-aarch64
GIC_VERSION = 3
MACHINE = virt,gic-version=$(GIC_VERSION),virtualization=on
ifndef NCPU
NCPU = 1
endif

OBJS = src/boot.o src/init.o src/uart.o src/lib.o src/pmalloc.o src/printf.o src/vcpu.o \
			 src/vm.o src/mm.o src/vector.o src/guest.o src/trap.o src/pcpu.o src/vgic.o \
			 src/gic.o src/mmio.o src/vtimer.o src/virtio.o src/virtio_net.o

QEMUOPTS = -cpu $(QCPU) -machine $(MACHINE) -smp $(NCPU) -m 256
QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -nographic -kernel mvmm

TAP_NUM = $(shell date '+%s')

MAC_H = $(shell date '+%H')
MAC_M = $(shell date '+%M')
MAC_S = $(shell date '+%S')

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

guest/hello.img: guest/Makefile
	make -C guest

-include: *.d

all: mvmm

mvmm: $(OBJS) src/memory.ld guest/hello.img
	$(LD) -r -b binary guest/hello.img -o hello-img.o
	$(LD) $(LDFLAGS) -T src/memory.ld -o $@ $(OBJS) hello-img.o

qemu: mvmm
	$(QEMU) $(QEMUOPTS)

dev: mvmm
	sudo ip link add br4mmvm type bridge || true
	sudo ip link set br4mmvm up || true
	sudo ip tuntap add dev tap$(TAP_NUM) mode tap
	sudo ip link set dev tap$(TAP_NUM) master br4mmvm
	sudo ip link set tap$(TAP_NUM) up
	$(QEMU) $(QEMUOPTS) -netdev tap,id=net0,ifname=tap$(TAP_NUM),script=no,downscript=no -device virtio-net-device,netdev=net0,mac=70:32:17:$(MAC_H):$(MAC_M):$(MAC_S),bus=virtio-mmio-bus.0
	sudo ip link set tap$(TAP_NUM) down
	sudo ip tuntap del dev tap$(TAP_NUM) mode tap

dev1: mvmm
	$(QEMU) $(QEMUOPTS) -netdev tap,id=net0,ifname=tap0,script=no,downscript=no -device virtio-net-device,netdev=net0,mac=90:19:39:59:79:99,bus=virtio-mmio-bus.0

dev2: mvmm
	$(QEMU) $(QEMUOPTS) -netdev tap,id=net1,ifname=tap1,script=no,downscript=no -device virtio-net-device,netdev=net1,mac=70:32:39:85:00:11,bus=virtio-mmio-bus.0

gdb: mvmm
	$(QEMU) -S -gdb tcp::1234 $(QEMUOPTS)

clean:
	make -C guest clean
	$(RM) $(OBJS) mvmm *.img *.o */*.d

.PHONY: qemu gdb clean all
