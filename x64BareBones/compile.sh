#!/bin/bash
# Compila el TP usando el docker de la catedra.
# Si todavia no se construyo el ModulePacker, lo hace antes (una vez).
set -e

DOCKER_IMG=agodio/itba-so-multiarch:3.1

# por defecto compilo el bitmap. si paso "buddy" como argumento, uso el buddy mm.
TARGET=all
if [ "$1" = "buddy" ]; then
    TARGET=buddy
fi

if [ ! -f Toolchain/ModulePacker/mp.bin ]; then
    echo ">>> Construyendo ModulePacker (primera vez)..."
    docker run --rm -v "$PWD":/root --security-opt seccomp=unconfined \
        "$DOCKER_IMG" bash -c "cd /root/Toolchain/ModulePacker && make"
fi

echo ">>> Compilando (target: $TARGET)..."
docker run --rm -v "$PWD":/root --security-opt seccomp=unconfined \
    "$DOCKER_IMG" bash -c "cd /root && make clean && make $TARGET"
