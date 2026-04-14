#!/bin/bash
# La parte a partir del -audiodev es para el sonido

#Para macOs: -audiodev coreaudio,id=speaker
#Para Linux: -audiodev pa,id=speaker -machine pcspk-audiodev=speaker

qemu-system-x86_64 -hda Image/x64BareBonesImage.qcow2 -m 512 -audiodev pa,id=speaker -machine pcspk-audiodev=speaker