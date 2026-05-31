#!/bin/bash
# Levanta QEMU con la imagen ya compilada.
if [ ! -f Image/x64BareBonesImage.qcow2 ]; then
    echo "No existe Image/x64BareBonesImage.qcow2. Corre ./compile.sh primero."
    exit 1
fi
qemu-system-x86_64 -hda Image/x64BareBonesImage.qcow2 -m 512

