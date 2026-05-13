#!/bin/bash
# Borra todos los .o, binarios e imagenes generadas.
docker run --rm -v "$PWD":/root --security-opt seccomp=unconfined \
    agodio/itba-so-multiarch:3.1 bash -c "cd /root && make clean"
