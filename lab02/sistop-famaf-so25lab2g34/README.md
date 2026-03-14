# Laboratorio 2: Semáforos en xv6 RISC-V – Sistemas Operativos 2025 – FaMAF – UNC
<a name="índice"></a>
&nbsp;
## Índice
- [1. Descripción del trabajo](#1-descripción-del-trabajo)
- [2. Instrucciones de compilación y ejecución](#2-instrucciones-de-compilación-y-ejecución)
- [3. Metodología del trabajo en equipo](#3-metodología-del-trabajo-en-equipo)
- [4. Syscalls de semáforos (uso)](#4-syscalls-de-semáforos-uso)
    - [4.1. `int sem_open(int sem, int value)`](#41-int-sem_openint-sem-int-value)
    - [4.2. `int sem_close(int sem)`](#42-int-sem_closeint-sem)
    - [4.3. `int sem_up(int sem)`](#43-int-sem_upint-sem)
    - [4.4. `int sem_down(int sem)`](#44-int-sem_downint-sem)
- [5. Descripción de archivos principales](#5-descripción-de-archivos-principales)
    - [5.1. En `kernel/`](#51-en-kernel)
    - [5.2. En `user/`](#52-en-user)
    - [5.3. En la raíz del repositorio](#53-en-la-raíz-del-repositorio)
- [6. Uso de asistentes de IA](#6-uso-de-asistentes-de-ia)
- [7. Decisiones de diseño](#7-decisiones-de-diseño)
- [8. Enlace al video](#8-enlace-al-video)
<a name="1-descripción-del-trabajo"></a>
&nbsp;
## 1. Descripción del trabajo

En este laboratorio, se implementa un sistema de **semáforos** como mecanismo de sincronización entre procesos para la versión RISC-V de [xv6](https://pdos.csail.mit.edu/6.1810/2025/xv6.html), un **sistema operativo de enseñanza tipo Unix**. Los semáforos se implementan en espacio de **kernel** y se exponen mediante syscalls accesibles desde espacio de usuario.

Se trata de **semáforos nombrados**, que presentan las siguientes características:

- Son administrados por el **kernel**.
- Están disponibles **globalmente** para todos los procesos del sistema operativo.
- Su estado se preserva mientras el SO esté activo (se pierden al reiniciar el sistema).
- Cada semáforo se identifica mediante un número entero entre `0` y un límite máximo (`NSEMAPHORE`), de manera similar a los descriptores de archivos.

El trabajo incluye la implementación de:

1. Cuatro syscalls: `sem_open()`, `sem_close()`, `sem_up()` y `sem_down()`.
2. Un programa de usuario llamado `pingpong` que utiliza los semáforos para funcionar de manera sincronizada.
<a name="2-instrucciones-de-compilación-y-ejecución"></a>
&nbsp;
## 2. Instrucciones de compilación y ejecución

En Linux, instalar el emulador QEMU ejecutando:

```bash
sudo apt-get install qemu-system-riscv64 gcc-riscv64-linux-gnu
```

Luego seguir estos pasos:

1. Clonar el repositorio.
2. Ubicarse en la raíz del repositorio (`so25lab2g34`).
3. Ejecutar `make qemu`. Esto compila xv6 y lo ejecuta en el emulador QEMU.
4. Ejecutar el programa de usuario `pingpong`, pasándole un entero positivo como único argumento, que indica cuántas veces se imprimirán las palabras "ping" y "pong".

```bash
$ pingpong 2
ping
    pong
ping
    pong
```

5. Para salir: presionar `Ctrl+a` y luego `x`.
6. Para borrar los ejecutables: ejecutar `make clean`.
<a name="3-metodología-del-trabajo-en-equipo"></a>
&nbsp;
## 3. Metodología del trabajo en equipo

**Integrantes:**

* Gerbaudo, Nicolás Ignacio
* Ontivero, Nahuel Mauricio
* Vides, Alejo Miguel

La primera parte del trabajo, realizada de manera individual, consistió en:

- preparar la **estructura básica de las syscalls**, esto es, su soporte en el kernel y en espacio de usuario;
- implementar **versiones dummy** de:
    - las funciones internas del kernel que realizan la lógica real de las syscalls, y
    - del programa de usuario `pingpong`.

Estas implementaciones permitieron compilar y ejecutar correctamente xv6; hasta aquí, las syscalls sólo retornaban `1` y `pingpong` sólo imprimía unas pocas líneas de prueba.

La segunda parte del trabajo, realizada de manera grupal mediante videollamadas por Discord, consistió en desarrollar la implementación definitiva de esas funciones internas del kernel y del programa `pingpong`.

Finalmente, pequeñas modificaciones y correcciones fueron realizadas de manera individual.

El *coding style* se mantuvo consistente con las convenciones de xv6.

Para el video de demostración, se redactó un guión dividido en tres partes; cada integrante grabó su sección de forma independiente y luego se combinaron las grabaciones para obtener el resultado final.
<a name="4-syscalls-de-semáforos-uso"></a>
&nbsp;
## 4. Syscalls de semáforos (uso)

El sistema dispone de `NSEMAPHORE` semáforos (`NSEMAPHORE` está definido como `64`).

Cada semáforo se identifica con un ID, que es un entero entre `0` y `NSEMAPHORE - 1`.

Al iniciar el sistema, todos los semáforos están cerrados.

Un proceso puede:

1. **abrir** un semáforo cerrado asignándole un valor inicial no negativo (cantidad de recursos disponibles que múltiples procesos pueden compartir),
2. **cerrar** un semáforo abierto,
3. **incrementar** el valor de un semáforo abierto (*liberar* un recurso),
4. **decrementar** el valor de un semáforo abierto (*ocupar* un recurso).

Cada una de estas operaciones se implementó mediante una syscall, que retorna

- `1` en caso de **éxito** y
- `0` en caso de **error**.
<a name="41-int-sem_openint-sem-int-value"></a>
&nbsp;
### 4.1. *int sem_open(int sem, int value)*

**Uso**

Se usa para **abrir** un semáforo cerrado asignándole un valor inicial no negativo (cantidad de recursos disponibles que múltiples procesos pueden compartir).

**Parámetros**

- `sem` es el ID del semáforo que se quiere abrir.
- `value` es el valor inicial que se le quiere dar al semáforo.

**Error**

Tiene tres casos de error:

1. `sem` no es un ID válido (i.e., `sem < 0 || sem >= NSEMAPHORE`).
2. `value` no es un valor válido (i.e., `value < 0`).
3. El semáforo `sem` ya está abierto.

En cualquier caso, retorna `0`.

**Éxito**

Asigna el valor `value` al semáforo `sem` y retorna `1`.
<a name="42-int-sem_closeint-sem"></a>
&nbsp;

### 4.2. *int sem_close(int sem)*

**Uso**

Se usa para **cerrar** un semáforo abierto.

**Parámetros**

- `sem` es el ID del semáforo que se quiere cerrar.

**Error**

Tiene dos casos de error:

1. `sem` no es un ID válido (i.e., `sem < 0 || sem >= NSEMAPHORE`).
2. El semáforo `sem` ya está cerrado.

En cualquier caso, retorna `0`.

**Éxito**

Cierra el semáforo `sem` y retorna `1`.
<a name="43-int-sem_upint-sem"></a>
&nbsp;

### 4.3. *int sem_up(int sem)*

**Uso**

Se usa para **incrementar** el valor de un semáforo abierto (*liberar* un recurso).

**Parámetros**

- `sem` es el ID del semáforo abierto cuyo valor se quiere incrementar.

**Error**

Tiene tres casos de error:

1. `sem` no es un ID válido (i.e., `sem < 0 || sem >= NSEMAPHORE`).
2. El semáforo `sem` está cerrado.
3. El valor del semáforo `sem` es `2147483647` (i.e., `2^31 - 1`).

En cualquier caso, retorna `0`.

**Éxito**

Incrementa el valor del semáforo `sem` (*libera* un recurso), despierta a los procesos que están esperando decrementarlo (*ocupar* el recurso) y retorna `1`.
<a name="44-int-sem_downint-sem"></a>
&nbsp;

### 4.4. *int sem_down(int sem)*

**Uso**

Se usa para **decrementar** el valor de un semáforo abierto (*ocupar* un recurso).

**Parámetros**

- `sem` es el ID del semáforo abierto cuyo valor se quiere decrementar.

**Error**

Tiene dos casos de error:

1. `sem` no es un ID válido (i.e., `sem < 0 || sem >= NSEMAPHORE`).
2. El semáforo `sem` está cerrado.

En cualquier caso, retorna `0`.

**Éxito**

Mientras el valor del semáforo `sem` sea `0` (i.e., no haya recursos disponibles), el **proceso que la llamó** duerme hasta que **otro proceso**

- incremente el valor del semáforo (*libere* un recurso) y
- lo despierte.

Al ser despertado y encontrar un recurso disponible, decrementa el valor del semáforo (*ocupa* el recurso) y retorna `1`.
<a name="5-descripción-de-archivos-principales"></a>
&nbsp;

## 5. Descripción de archivos principales

A continuación, se presenta un resumen del propósito de cada archivo modificado (**M**) o creado (**C**).
<a name="51-en-kernel"></a>
&nbsp;
### 5.1. *En kernel/*

1. `kernel/param.h` (**M**)

Define parámetros constantes que afectan a todo el kernel. Aquí se definió el máximo número de semáforos:

```c
#define NSEMAPHORE   64  // maximum number of semaphores
```

2. `kernel/semaphore.c` (**C**)

- Define el tipo semáforo:

```c
struct semaphore {
  struct spinlock lock;
  int value;
};
```

- Declara la tabla de semáforos:

```c
static struct semaphore semaphore[NSEMAPHORE];
```

- Implementa las funciones internas del kernel:
    - `void seminit(void);`
    - `int sem_open(int sem, int value);`
    - `int sem_close(int sem);`
    - `int sem_up(int sem);`
    - `int sem_down(int sem);`

La función `seminit()` inicializa la tabla de semáforos y las funciones `sem_*()` implementan la lógica real de las syscalls, usando `acquire()`, `release()`, `wakeup()` y `sleep()`.

3. `kernel/semaphore.h` (**C**)

Declara las funciones `sem_*()` implementadas en `kernel/semaphore.c`.

4. `kernel/defs.h` (**M**)

Declara funciones y tipos usados por varios módulos. Aquí se agregó una sección donde se declara la función que inicializa la tabla de semáforos. Esto permite que otros módulos del kernel llamen a `seminit()`.

5. `kernel/main.c` (**M**)

Contiene la preparación y el punto de arranque del kernel. Aquí se agregó la llamada a `seminit()` para inicializar la tabla de semáforos.

6. `kernel/sysproc.c` (**M**)

Implementa los *manejadores* de syscalls (i.e., las funciones que manejan las syscalls) relacionados con **procesos**. En donde se agregó:

```c
#include "semaphore.h"
```

- También se implementaron los *manejadores* de syscalls:
    - `uint64 sys_sem_open(void);`
    - `uint64 sys_sem_close(void);`
    - `uint64 sys_sem_up(void);`
    - `uint64 sys_sem_down(void);`

Cada *manejador* obtiene el argumento (o los argumentos) de la syscall con la función de kernel `argint()` y llama a la función que hace el trabajo real (`sem_*()`).

7. `kernel/syscall.h` (**M**)

Define los identificadores de syscall. 

8. `kernel/syscall.c` (**M**)

Implementa la gestión de syscalls: mantiene el arreglo que mapea los identificadores de syscall con los *manejadores* de syscalls e implementa `syscall()`, que recibe llamadas desde modo usuario y ejecuta la función correspondiente, y también se agregaron los ítems correspondientes al arreglo que mapea los identificadores de las syscalls con sus respectivos manejadores. 
<a name="52-en-user"></a>
&nbsp;
### 5.2. *En user/*

1. `user/user.h` (**M**)

Declara las syscalls y las funciones que pueden usar los programas de usuario.

2. `user/usys.pl` (**M**)

Script de Perl que genera automáticamente los *stubs* de syscalls que los programas de usuario pueden llamar.

Aquí se agregó una llamada a la subrutina `entry` para cada syscall:

```perl
entry("sem_open");
entry("sem_close");
entry("sem_up");
entry("sem_down");
```

Así, cuando se genera `user/usys.S` al compilar xv6, habrá *stubs* para cada syscall. Por ejemplo:

```asm
sem_open:
 li a7, SYS_sem_open
 ecall
 ret
```

- `li a7, SYS_sem_open`: carga en el registro `a7` el identificador de la syscall `sem_open()`;
- `ecall`: llama al **kernel** para ejecutar la syscall indicada en `a7`;
- `ret`: retorna al código de **usuario**, pasando cualquier valor devuelto por la syscall en `a0`.

3. `user/pingpong.c` (**C**)

Implementa el programa de usuario `pingpong` usando las syscalls `sem_open()`, `sem_close()`, `sem_up()` y `sem_down()`. El programa recibe un entero positivo como único argumento, que indica cuántas veces se imprimirán las palabras "ping" y "pong".

```bash
$ pingpong 3
ping
    pong
ping
    pong
ping
    pong
```
<a name="53-en-la-raíz-del-repositorio"></a>
&nbsp;
### 5.3. *En la raíz del repositorio*
1. `Makefile` (**M**)

Aquí se agregaron:
- `semaphore.o` a la lista de **objetos del kernel** para que la función `seminit()` y las demás funciones `sem_*()` sean compiladas correctamente.
- `_pingpong` a la lista de **programas de usuario**. De esta forma, se puede generar el ejecutable `pingpong`.

2. `README.md` (**C**)
<a name="6-uso-de-asistentes-de-ia"></a>
&nbsp;

## 6. Uso de asistentes de IA

Se utilizaron herramientas de inteligencia artificial para:

- consultar bibliografía y material de referencia,
- resolver dudas sobre el uso de funciones de xv6,
- proponer nombres más claros para variables,
- mejorar la redacción de comentarios y mensajes de `printf`, y
- asistir en la elaboración de la documentación.
<a name="7-decisiones-de-diseño"></a>
&nbsp;

## 7. Decisiones de diseño
- A la función `seminit()` la hicimos **pública para todo el kernel** para que el módulo `main.c` pueda llamarla e inicializar la tabla al arrancar xv6.
- Además, es la **única** función de `semaphore.c` que hicimos pública, porque las demás no son necesarias en otros módulos del kernel.
- La estructura del semáforo sólo cuenta con el campo `spinlock` y el de recursos, ya que `-1` significa "cerrado". Los recursos no pueden exceder `2^31 - 1` para evitar un *overflow*.
- Originalmente, estábamos generando un un proceso nuevo por "ping", pero al releer el enunciado modificamos la implementación para que **un proceso** imprima "ping" `N` veces y el otro imprima `N` veces "pong".
- También al principio utilizábamos el semáforo `0` para `ping` y el `1` para `pong`, pero en la versión final recorremos la tabla de semáforos hasta encontrar el primer semáforo cerrado disponible.
<a name="8-enlace-al-video"></a>
&nbsp;

## 8. Enlace al video

[Entrega Laboratorio 2 Sistemas Operativos 2025 FAMAF (YouTube)](google.com).
