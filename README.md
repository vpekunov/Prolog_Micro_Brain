# Prolog MicroBrain (the simplified analogue of GNU Prolog) Interpreter 

***Может компилироваться как MS Visual C++ так и GNU C++***

В ***MS Visual C++*** компилируется проект prolog_micro_brain.sln

В ***GNU C++*** для компиляции можно дать команду: g++ -o prolog_micro_brain prolog_micro_brain.cpp -O4 -std=c++0x -msse4

Папка ***tests*** содержит простые примеры. Примеры иожно запустить двумя способами:
1. ***Из командной строки***. Например, если необходимо запустить пример ***prog.pro***
со стартом предиката ***start***, можно дать команду: ./prolog_micro_brain -consult prog.pro start
2. ***Из интерпретатора***. Например, если необходимо запустить пример ***prog.pro***
со стартом предиката ***start***, можно запустить файл ./prolog_micro_brain без параметров и в появившемся
окне дать две команды:
   consult('prog.pro').
   start.
Для выхода из интерпретатора достаточно дать команду halt.
