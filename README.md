# Trabajo Práctico Especial de Arquitectura de Computadoras

## Integrantes
* Augusto Felipe Ospal Madeo (63669)
* Valentino Esteves (64335)
* Santiago Cibeira (64560)

## Manual de Usuario
Es requisito tener instalados los siguientes programas:
* gcc
* qemu
* docker
* make
* nasm

### Preparar el entorno
Una vez ubicado en el directorio deseado, desde la terminal ejecutar los siguientes comandos:

```bash
docker pull agodio/itba-so:2.0
git clone https://github.com/AugustoOspal/Arqui-TPE.git
cd Arqui-TPE/x64BareBones
docker run -d -v ${PWD}:/root --security-opt seccomp:unconfined -it --name TPE agodio/itba-so:2.0
```

### Compilación
Dentro del directorio Arqui-TPE/x64BareBones, ejecutar:
```bash
./compile.sh
```

### Ejecución
Dentro del directorio Arqui-TPE/x64BareBones, ejecutar:
```bash
# Para linux
./runLinux.sh

# Para macOS
./runMacOS.sh
```

### Limpieza de archivos temporales
Dentro del directorio Arqui-TPE/x64BareBones, ejecutar:
```bash
./clean.sh
```

### Cómo conseguir el hash MD5?

Descarga el ZIP del proyecto desde GitHub. En la terminal, ejecuta:

```bash
md5sum Arqui-TPE-master.zip
```

El número que se mostrará es el hash MD5.