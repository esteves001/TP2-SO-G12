
# Por defecto, el target del kernel es 'all'
KERNEL_TARGET ?= all

all:  bootloader kernel userland image

bootloader:
	cd Bootloader; make all

kernel:
	cd Kernel; make $(KERNEL_TARGET)

userland:
	cd Userland; make all

image: kernel bootloader userland
	cd Image; make all

# El target 'buddy' simplemente vuelve a llamar a make con la variable KERNEL_TARGET cambiada.
buddy:
	@$(MAKE) all KERNEL_TARGET=buddy

clean:
	cd Bootloader; make clean
	cd Image; make clean
	cd Kernel; make clean
	cd Userland; make clean

.PHONY: bootloader image collections kernel userland all clean buddy
