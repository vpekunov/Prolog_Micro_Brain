# Prolog MicroBrain (the simplified analogue of GNU Prolog) Interpreter 

***Может компилироваться как MS Visual C++ так и GNU C++***

В ***MS Visual C++*** компилируется проект prolog_micro_brain.sln
С помощью ***Free Pascal*** компилируется проект xpathInduct.dir/xpathInduct.lpr

В ***Linux*** для компиляции можно дать команду: sh ./install.sh

Папка ***tests*** содержит простые примеры. Примеры можно запустить двумя способами:
1. ***Из командной строки***. Например, если необходимо запустить пример ***prog.pro***
со стартом предиката ***start***, можно дать команду: ./prolog_micro_brain -consult prog.pro start
2. ***Из интерпретатора***. Например, если необходимо запустить пример ***prog.pro***
со стартом предиката ***start***, можно запустить файл ./prolog_micro_brain без параметров и в появившемся
окне дать две команды:

   consult('prog.pro').

   start.

Для выхода из интерпретатора достаточно дать команду halt.

***Основные поддерживаемые предикаты и операции***
- ()
- \\+
- ;
- =..
- ->
- =
- ==
- <
- \>
- \\=
- is (поддерживаются + - * / и функции max, abs, round)
- once
- call
- append
- member
- last
- length
- atom_length
- nth
- atom_concat
- atom_chars
- number_atom
- number
- consistency
- listing
- current_predicate
- predicate_property
- $predicate_property_pi
- g_assign
- g_read
- g_assign_nth
- fail
- true
- open
- close
- get_char
- peek_char
- read_token
- write
- nl
- seeing
- telling
- seen
- told
- see
- tell
- open_url
- track_post
- consult
- asserta
- assertz
- retract
- retractall
- inc
- halt
- var
- load_classes
- init_xpathing
- induct_xpathing
- unload_classes
- add_object
- add_link
- del_object
- del_link
- del_all_objects
- prepare_model_for_induct
- import_model_after_induct
- atom_hex
- atom_hexs
- page_id
- unset
- add
- mul
- div
- regularize
- to_chain
- nnewff
- nload
- nsave
- nsimplify
- train
- sim
- optimize_load
- parallelize_level
