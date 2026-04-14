include Makefile.inc

.PHONY: all clean SampleCodeModule sampleDataModule

all: SampleCodeModule sampleDataModule

# primero compilamos la librería que ahora está dentro de SampleCodeModule
SampleCodeModule:
	$(MAKE) -C SampleCodeModule/UserLib
	$(MAKE) -C SampleCodeModule

sampleDataModule:
	printf "This is sample data." > 0001-sampleDataModule.bin
	dd if=/dev/zero bs=1 count=1 >> 0001-sampleDataModule.bin

clean:
	$(MAKE) -C SampleCodeModule/UserLib clean
	$(MAKE) -C SampleCodeModule clean
	rm -f 0001-sampleDataModule.bin