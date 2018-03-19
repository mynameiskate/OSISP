# 10. Написать скрипт, находящий все дубликаты (с одинаковым содержимым) 
#файлов в заданном диапазоне размеров от N1 до N2 
#(N1, N2 задаются в аргументах командной строки),
#начиная с исходного каталога и ниже.
# Имя исходного каталога задаётся пользователем в 
#качестве первого аргумента командной строки. 

#!/bin/bash

ERRORS="/tmp/errors.txt"

TEMP=$IFS
IFS=$'\n'

FILE_LIST=$( find "$1" -type f -size "+$2c" -size "-$3c" 2>$ERRORS )

for i in $FILE_LIST
do
	for j in $FILE_LIST
	do
		if  cmp -s $i $j && [ $i != $j ]
		then
			echo "$i = $j"
		fi
	done
done  

sed -i "s/.[A-Za-z]*:/`basename $0`:/" $ERRORS 
cat $ERRORS >&2

rm $ERRORS
IFS=$TEMP



