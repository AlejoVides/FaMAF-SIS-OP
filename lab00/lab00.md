	https://explainshell.com/
# *GREP: print lines matching a pattern*
grep searches the named input FILEs (or standard input if no files are named, or if a single hyphen-minus (-) is given as file name) for lines containing a match to the given PATTERN. By default, grep prints the matching lines.
# *CAT: concatenate files and print on the standard output*
Concatenate FILE(s), or standard input, to standard output.
# *SORT: sort lines of text files*
# *HEAD: output the first part of files*
Print the first 10 lines of each FILE to standard output. With more than one FILE, precede each with a header giving the file name. With no FILE, or when FILE is -, read standard input.
# *AWK: pattern scanning and processing language*

# *TR: translate or delete characters*
Translate, squeeze, and/or delete characters from standard input, writing to standard output.
# *CUT: remove sections from each line of files*
remove sections from each line of files.
# *WC: print newline, word, and byte counts for each file*
Print newline, word, and byte counts for each FILE, and a total line if more than one FILE is specified. With no FILE, or when FILE is -, read standard input. A word is a non-zero-length sequence of characters delimited by white space. The options below may be used to select which counts are printed, always in the following order: newline, word, character, byte, maximum line length.
# *CURL: transfer a URL*

# *SED: stream editor for filtering and transforming text*

# *IP: show / manipulate routing, devices, policy routing and tunnels*

# *MKDIR: make directories*
Create the DIRECTORY(ies), if they do not already exist.
# *TOUCH: change file timestamps*
change only the access time
# *MV: move (rename) files*
Rename SOURCE to DEST, or move SOURCE(s) to DIRECTORY.

---

EJ1: Información del procesador
	¿Qué modelo de procesador tiene tu equipo? Cuando necesitamos un dato del equipo, como por ejemplo el modelo del procesador, podemos buscar en el archivo /proc/cpuinfo. Sin embargo, este comando nos devuelve un listado muy largo y tienen que encontrar la forma de buscar sólo la información necesaria.
	Comandos útiles: cat, grep, head Objetivo: Mostrar sólo el modelo del procesador (una línea).

```bash
cat /proc/cpuinfo | grep model | grep name
```

EJ2: Número de cores
	Si la computadora tiene más de una unidad de ejecución (multi-core) seguramente en el punto anterior se repitió más de una vez el modelo del procesador. Averiguar cómo usar wc para poder determinar cuántas unidades de ejecución tiene el procesador, aprovechando los comandos utilizados en el ejercicio 1.
	Comandos útiles: cat, grep, wc Objetivo: Mostrar sólo el número de cores (un número).

```bash
cat /proc/cpuinfo | grep model | grep name | wc -l
```

EJ3: Red social de superhéroes
	Usted tiene la maravillosa idea de crear una red social para superhéroes donde puedan interactuar sin máscaras y con sus nombres verdaderos (todo financiado por Bruce Wayne o Tony Stark, dependiendo del estudio). Para la semana de lanzamiento, le enviará una invitación personalizada a cada superhéroe. Su base de datos es el siguiente archivo heroes.csv.
	Notar que: La segunda columna contiene el nombre real de cada superhéroe, pero los usuarios en su sistema deben estar todos en minúsculas y sin espacios (deben separar los nombres y apellidos con el carácter “ _ ” . ). ¿Cómo obtendría esta lista de nombres de usuarios con un único comando bash? Incluso la descarga del archivo. Puntos extra si eliminan las líneas en blanco del resultado. Los que no tienen identidad(Identity) no incluirlos en el listado final.
	Comandos útiles: curl, cut, tr, grep, sed.
	Objetivo: Crear un archivo superheroes_usuarios.txt con los nombres procesados. 

```bash
curl https://raw.githubusercontent.com/dariomalchiodi/superhero-datascience/master/content/data/heroes.csv | cut -d";" -f1,2 | grep -v ";"$ | cut -d";" -f1 | tr '[:upper:]' '[:lower:]' | tr ' ' '_' | cat > out.txt
```

EJ4A: Día de máxima temperatura
	El archivo weather_cordoba.in contiene mediciones meteorológicas realizadas en un día en Córdoba. Las primeras 3 columnas corresponden al año, mes y día de las mediciones. Las restantes 6 columnas son la temperatura media, la máxima, la mínima, la presión atmosférica, la humedad y las precipitaciones medidas de ese día.
	Objetivo: Calcular el día de máxima temperatura (máximo de máximas). Idea para facilitar la tarea: Ordenar los días según su temperatura máxima. Puntos extra por mostrar en la pantalla sólo la fecha de dicho día.
	⚠ Importante: Para evitar los números reales, los grados están expresados en décimas de grados (por ejemplo, 15.2 grados está representado por 152 décimas). La presión también ha sido multiplicada por 10 y las precipitaciones por 100, o sea que están expresadas en centésimas de milímetro.
	Comandos útiles: sort, head, awk.
	Objetivo: Mostrar la fecha del día más caluroso.

```bash
cat weather_cordoba.in | cut -d" " -f1,2,3,5 | sort -n -r -k4 | head -n 1| cut -d" " -f4 --complement
```

EJ4B: Día de mínima temperatura
	Objetivo: Calcular el día de mínima temperatura (mínimo de mínimas). Idea para facilitar la tarea: Ordenar los días según su temperatura mínima. Puntos extra por mostrar en la pantalla sólo la fecha de dicho día. Comandos útiles: sort, head, awk Objetivo: Mostrar la fecha del día más frío.

```bash
cat weather_cordoba.in | cut -d" " -f1,2,3,6 | sort -n -k4 | head -n 1| cut -d" " -f4 --complement
```

EJ5: Ranking de tenistas
	El archivo atpplayers.in es un listado por orden alfabético de jugadores profesionales de tenis. El nombre del jugador viene acompañado de una abreviatura de su país, el número que ocupa en el ranking, su edad, su puntaje y el número de torneos jugados en el último año.
	Objetivo: Ordenar el listado de jugadores según la posición en el ranking.
	Comandos útiles: sort Nota: El ranking está en la tercera columna.

```bash
cat atpplayers.in | cut -d" " -f1,3 | sort -n -k2
```

EJ6: Tabla de Superliga
	El archivo superliga.in contiene datos con el siguiente formato: nombre de equipo sin espacios, puntos, partidos jugados, partidos ganados, partidos empatados, partidos perdidos, goles a favor y goles en contra.
	Objetivo: Ordenar la tabla del campeonato de la Superliga según la cantidad de puntos, y desempatar por diferencia de goles.
	Comandos útiles: sort, awk Nota: La diferencia de goles se calcula como (goles a  favor - goles en contra).

```bash
cat superliga.in | cut -d" " -f1,2,7,8 | awk -F ' ' '{print $1, $2, $3-$4}' | sort -n -r -k3 | sort -n -r -k2 | cut -d" " -f1
```

EJ7: MAC address
	¿Cómo ver la MAC address de nuestro equipo? Investiguen el comando ip. En el manual de grep van a encontrar la especificación de muchas operaciones, por ejemplo -o, -i, y muchas más. Algo muy utilizado son las expresiones regulares para realizar una búsqueda. En el manual de grep tienen un apartado donde explica su uso. Con esta información deberían poder construir una secuencia de comandos de shell para imprimir por consola la MAC address de su interfaz ethernet.
	Comandos útiles: ip, grep, expresiones regulares Objetivo: Mostrar solo la MAC address de la interfaz ethernet.

```bash
ip -o link | grep "link/ether" | sed 's/.*link\/ether /MAC address /' | cut -b 1-29
```

EJ8: Renombrar archivos de subtítulos
	Supongamos que bajaron una serie de televisión completa con subtítulos (de forma completamente legal, por supuesto). Sin embargo, los subtítulos tienen el sufijo _es en el nombre de cada archivo y para que puedan ser reproducidos en el televisor, que nunca fue demasiado smart, el archivo de subtítulos tiene que tener exactamente el mismo nombre que el archivo de video con la extensión srt. La serie tiene más de 100 capítulos, es imposible realizar los cambios uno a uno.
	Parte 1: Para emular la situación, crear una carpeta con el nombre serie_prueba y dentro de ella crear 10 archivos con nombres que sigan el formato fma_S01EXX_es.srt, donde XX es el número de capítulo, desde 1 hasta 10.
	Parte 2: Con un segundo comando (el que usarían realmente), cambiar el nombre de cada archivo sacando el sufijo _es.
	Comandos útiles: mkdir, touch, for, mv Ayuda: Usar un ciclo for. Se pueden hacer ciclos en una única línea. Puntos extra si los archivos de video no siguen un patrón, sino que pueden llamarse de cualquier forma.

```bash
# mkdir -p serie_prueba && for i in {1..10}; do ep=$(printf "%02d" $i); touch serie_prueba/fma_S01E${ep}_es.srt; done
for file in serie_prueba/*_es.srt; do mv "$file" "${file%_es.srt}.srt"; done
```

---

	vides_alejo_44580473.sh
```bash
#!/bin/bash
# EJERCICIO 1: Modelo del procesador
cat /proc/cpuinfo | grep model | grep name
# EJERCICIO 2: Número de unidades de ejecución (cores)
cat /proc/cpuinfo | grep model | grep name | wc -l
# EJERCICIO 3: Lista de nombres de usuarios para red social de superhéroes
curl https://raw.githubusercontent.com/dariomalchiodi/superhero-datascience/master/content/data/heroes.csv | cut -d";" -f1,2 | grep -v ";"$ | cut -d";" -f1 | tr '[:upper:]' '[:lower:]' | tr ' ' '_'
# EJERCICIO 4A: Día de máxima temperatura en Córdoba
cat weather_cordoba.in | cut -d" " -f1,2,3,5 | sort -n -r -k4 | head -n 1| cut -d" " -f4 --complement
# EJERCICIO 4B: Día de mínima temperatura en Córdoba
cat weather_cordoba.in | cut -d" " -f1,2,3,6 | sort -n -k4 | head -n 1| cut -d" " -f4 --complement
# EJERCICIO 5: Ordenar jugadores de tenis por ranking
cat atpplayers.in | cut -d" " -f1,3 | sort -n -k2
# EJERCICIO 6: Ordenar tabla de Superliga por puntos y diferencia de goles
cat superliga.in | cut -d" " -f1,2,7,8 | awk -F ' ' '{print $1, $2, $3-$4}' | sort -n -r -k3 | sort -n -r -k2 | cut -d" " -f1
# EJERCICIO 7: MAC address de la placa WiFi del equipo
ip -o link | grep "link/ether" | sed 's/.*link\/ether /MAC address /' | cut -b 1-29
# EJERCICIO 8: Renombrar archivos de subtítulos
for file in serie_prueba/*_es.srt; do mv "$file" "${file%_es.srt}.srt"; done
# EJERCICIO 9A: Recortar video con ffmpeg
echo no lo hice
# EJERCICIO 9B: Mezclar audio con ffmpeg
echo no lo hice
```
