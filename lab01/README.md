# Lab 1: MYBASH Sistemas Operativos 2025 FaMAF
---
## Instrucciones de Compilación y Ejecución

* Clona el repositorio y accede al directorio raíz.
* Compilar el proyecto ejecutando:

   ```bash
   make
   ```

* Ejecutar el MYBASH con:

   ```bash
   ./mybash
   ```

* Probar los tests unitarios:

```bash
make test-command
make test-parsing
make memtest
```

* O simplemente probar todos los tests juntos:

```bash
make test
```

---
## Descripción de Archivos Principales

- `mybash.c`: Programa principal, gestiona la lectura y realiza las llamadas a los módulos  de parseo y ejecución.

- `command.c & command.h`: Implementación de las estructuras y funciones para comandos simples y pipelines.

- `strextra.c & strextra.h`: Funciones auxiliares para manejo de cadenas.

- `parsing.c & parsing.h`: Funciones para el análisis sintáctico de la entrada del usuario.

- `builtin.c & builtin.h`: Implementación de comandos internos de MYBASH como cd help y exit.

- `execute.c & execute.h`: Lógica de ejecución de comandos y pipelines, incluyendo manejo de procesos.

- `tests/`: Carpeta con archivos de pruebas unitarias.

---
## Metodología de Trabajo en Equipo

- Integrantes:
	Gerbaudo Nicolás
	Ontivero Nahuel Mauricio
	Vides Alejo Miguel

Luego de hacer en equipo la primera parte del proyecto, en dónde definimos las estructuras, pudimos dividir el trabajo mediante Jira, en dónde asignamos subpartes de los modulos parsing, execute y builtin respectivamente.

Después de un commit inicial cotejamos en conjunto mediante pull requests hasta que todos los integrantes estuvieran conformes con el resultado para pushearlo a producción. También tuvimos reuniones semanales resolver dudas más complejas.

---
## Decisiones de diseño

* Todo después de un & es basura:
& le especifica a bash que debe correr un comando (o un grupo de comandos) en segundo plano y continuar con los siguientes. Nuestro bash toma todo lo viene después de & como basura

*  Argumentos con espacios rompen los TAD's:
Dado el enunciado y las herramientas dadas por la cátedra el parser no puede tomar argumentos con espacios entre medio.

```bash
grep proc/cpuinfo "Model Name"
```

Este ejemplo rompe totalmente a nuestro bash dado que _"Model_ es un argumento y _Name"_ es otro.

* Los comandos builtin solo pueden usarse solos:
En caso de que el usuario quiera usar los comandos cd help o exit tienen que ser el único scommand que tome bash.

---
## Uso de Asistentes de IA

Fue tentadora la idea de usar asistentes para generar documentación, ayudar con el scripting de la presentación del video y comentarios al código pero no fue necesario.

Si fue útil en los casos de buscar documentación, o implementaciones específicas de las sys calls en el momento de trabajar en execute.c. Pero en múltiples casos fue más una molestia que una ayuda.

---
## Enlace al Video

[Video de entrega](google.com)