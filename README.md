# TP2 — Sistemas Operativos (72.11) · Grupo 12

Construcción del núcleo de un sistema operativo monolítico de 64 bits con
administración de memoria física, scheduling preemptivo, sincronización por
semáforos e IPC mediante pipes. Hereda la base de hardware (drivers de teclado
y video, bootloader) del TPE de Arquitectura de Computadoras (x64BareBones).

Todo el código del proyecto vive dentro de la carpeta `x64BareBones/`.

## Integrantes

| Nombre              | Legajo |
|---------------------|--------|
| Matías Holgado      | 64747  |
| Valentino Esteves   | 64335  |
| Gerónimo Naso       | 64177  |

## Requisitos

- Docker (imagen de cátedra `agodio/itba-so-multiarch:3.1`)
- QEMU (`qemu-system-x86_64`)

La compilación ocurre **siempre** dentro del contenedor de cátedra. QEMU se
ejecuta en el host.

## Compilación

El proyecto soporta dos administradores de memoria física, seleccionables en
**tiempo de compilación**. Parados en `x64BareBones/`:

```bash
cd x64BareBones

# Bitmap (manager elegido por el grupo) — opción por defecto
./compile.sh

# Buddy system
./compile.sh buddy
```

`compile.sh` construye el ModulePacker la primera vez y luego corre
`make clean && make [buddy]` dentro del Docker. Equivalente manual:

```bash
docker run --rm -v "$PWD":/root --security-opt seccomp=unconfined \
    agodio/itba-so-multiarch:3.1 bash -c "cd /root && make clean && make"
# (o 'make buddy' para el buddy system)
```

## Ejecución

```bash
cd x64BareBones
./run.sh
```

Levanta QEMU con la imagen ya compilada (`Image/x64BareBonesImage.qcow2`).

## Limpieza

```bash
cd x64BareBones
./clean.sh
```

---

## Uso del shell (`sh`)

Al arrancar aparece el intérprete de comandos. Escribí `help` para ver la lista
completa y `test` para el listado de tests de la cátedra.

### Caracteres especiales

| Símbolo | Significado                                                        |
|---------|--------------------------------------------------------------------|
| `&`     | Al final de la línea: ejecuta el comando en **background**.         |
| `\|`    | Entre dos comandos: conecta el stdout del primero con el stdin del segundo mediante un pipe. |

> Solo se conectan **2** procesos por pipe (no soporta `p1 \| p2 \| p3`), tal
> como permite el enunciado.

### Atajos de teclado

| Atajo    | Acción                                                  |
|----------|---------------------------------------------------------|
| `Ctrl+C` | Mata al proceso en foreground (o cancela la línea actual si no hay ninguno). |
| `Ctrl+D` | Envía EOF al proceso que está leyendo de stdin.         |
| `Ctrl+R` | Captura snapshot de registros del CPU (ver `registers`). |

---

## Comandos

### Generales
| Comando      | Descripción                                             |
|--------------|---------------------------------------------------------|
| `help`       | Lista los comandos disponibles y los tests de cátedra.  |
| `test`       | Lista los tests provistos.                              |
| `clear`      | Limpia la pantalla.                                     |
| `date`       | Imprime fecha y hora del RTC.                           |
| `registers`  | Imprime el snapshot de registros (capturado con `Ctrl+R`). |
| `zoom_in` / `zoom_out` | Cambia el tamaño de fuente del video.         |
| `busywait`   | Busy-wait en userland (~10⁹ iteraciones). Útil para capturar `Ctrl+R`. |
| `busywaitkernel` | Busy-wait de 5 s en kernel (via `sleepMilli`).       |
| `exception1` | Dispara división por cero (prueba manejo de excepciones). |
| `exception2` | Dispara opcode inválido (prueba manejo de excepciones). |

### Memoria
| Comando | Descripción                                          |
|---------|------------------------------------------------------|
| `mem`   | Imprime el estado de la memoria (total / usada / libre). |

### Procesos
| Comando            | Descripción                                            |
|--------------------|--------------------------------------------------------|
| `ps`               | Lista los procesos (PID, nombre, estado, prioridad, foreground). |
| `loop`             | Imprime su PID con un saludo periódico (espera **activa**). |
| `kill <pid>`       | Mata el proceso con ese PID.                           |
| `nice <pid> <prio>`| Cambia la prioridad (1–5) de un proceso.               |
| `block <pid>`      | Alterna entre bloqueado y listo el proceso indicado.   |

### IPC
| Comando                       | Descripción                                              |
|-------------------------------|----------------------------------------------------------|
| `cat`                         | Imprime su stdin tal cual lo recibe.                     |
| `wc`                          | Cuenta las líneas del input.                             |
| `filter`                      | Filtra (elimina) las vocales del input.                  |
| `mvar <escritores> <lectores>`| Problema de múltiples lectores/escritores sobre una variable global (estilo MVar). Cada lector imprime en un color distinto. |

---

## Tests de la cátedra

Todos corren como **procesos de usuario** y aceptan `&` para background.

| Test                                   | Parámetros                                  | Descripción |
|----------------------------------------|---------------------------------------------|-------------|
| `test_mm <bytes>`                      | máximo de memoria a pedir, en bytes         | Pide y libera bloques aleatorios verificando que no se solapen. Solo imprime ante errores. |
| `test_proc <max_procesos>`             | cantidad máxima de procesos                 | Crea, bloquea, desbloquea y mata procesos dummy al azar. Solo imprime ante errores. |
| `test_prio <target>`                   | valor al que cuentan las variables          | 3 procesos que incrementan una variable: primero con la misma prioridad, luego con prioridades distintas. |
| `test_sync <pares> <loops> <use_sem>`  | pares inc/dec · iteraciones · `1`/`0` semáforos | Incrementan/decrementan una variable global. Con semáforos el resultado final es 0; sin ellos varía. |
| `test_pipe`                            | (sin parámetros)                            | Demo de productor/consumidor no emparentados sobre un pipe compartido por ID. |

---

## Tests internos del kernel (durante el desarrollo)

Además de los tests de la cátedra (que corren como procesos de usuario), durante
el desarrollo escribimos algunas pruebas a **nivel kernel** para validar cada
mecanismo de forma aislada, **antes** de exponerlo como comando de la shell.

No son tests unitarios automatizados: son pruebas manuales que se activan
descomentando un `#define` en `Kernel/kernel.c` y recompilando. En lugar de
levantar el userland normal, el kernel lanza los procesos de prueba y el
resultado se observa directamente en pantalla.

| Toggle (`#define` en `kernel.c`) | Qué valida |
|----------------------------------|------------|
| `SCHED_TEST` | Context switch: dos procesos (`test_a`/`test_b`) dibujan en paralelo dos líneas (de A's y B's). Si solo avanza una, el cambio de contexto no funciona. |
| `ARGS_TEST`  | Pasaje de parámetros: `test_args` recibe `argc`/`argv` y los dibuja, confirmando que llegan por `RDI`/`RSI`. |
| `PRIO_TEST`  | Scheduler con prioridades: 3 procesos cuentan hasta N con prioridades 1, 3 y 5; el de mayor prioridad termina primero. |

Nos sirvieron para dos cosas: confirmar que lo implementado en el kernel
funcionaba bien, y aislar errores. Una vez que estas pruebas pasaban, si algo
fallaba al usarlo desde userland sabíamos que el problema **no estaba en el
kernel**, lo que acotaba mucho la búsqueda. Por defecto todos los toggles están
comentados, así que el build normal levanta la shell.

---

## Ejemplos (fuera de los tests)

```sh
# Memoria
mem

# Procesos en background y gestión
loop &              # lanza loop en background, imprime [bg] pid N
ps                  # debería listar loop corriendo
nice 4 5            # sube la prioridad del pid 4
block 4             # lo bloquea; otro block 4 lo despierta
kill 4              # lo mata

# Pipes / transparencia stdin-stdout
cat                 # escribí texto; Ctrl+D para cerrar
ps | wc             # cuenta las líneas que imprime ps
filter              # escribí texto con vocales; Ctrl+D

# Sincronización
mvar 2 2            # esperado: ABABAB... ; matá un escritor y seguirá AAAA...

# Tests en foreground y background
test_mm 1048576
test_sync 3 1000 1  # con semáforos -> resultado 0
test_sync 3 1000 0  # sin semáforos -> condición de carrera
test_proc 10 &
```

---

## Limitaciones y requerimientos faltantes

Todos los requerimientos del enunciado están implementados. No hay funcionalidad
faltante ni parcialmente implementada.

Limitaciones conocidas:

- El administrador **bitmap** trabaja a nivel de página (4 KB): los pedidos se
  redondean hacia arriba, con la fragmentación interna que eso implica.
- En `mvar`, matar a un proceso **justo dentro de su sección crítica** (entre
  tomar y devolver el token de la variable) puede perder el token y producir
  deadlock. Es una ventana de pocas instrucciones; se mitiga limpiando procesos
  y semáforos viejos al relanzar el comando.
- `waitpid` espera a que termine un PID dado; no valida estrictamente la
  relación padre/hijo.

## Uso de inteligencia artificial

Durante el desarrollo se utilizaron herramientas de inteligencia artificial como
apoyo de investigación, para responder dudas conceptuales sobre los temas de la
materia, como ayuda puntual en la implementación de algunas partes y en la
redacción de este README. Se utilizo en gran parte como herramienta de debug cuando las implementaciones no funcionaban
