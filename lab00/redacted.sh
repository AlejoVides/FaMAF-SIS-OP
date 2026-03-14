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