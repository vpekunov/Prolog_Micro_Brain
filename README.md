# Prolog MicroBrain (the simplified analogue of GNU Prolog) Interpreter 

***����� ��������������� ��� MS Visual C++ ��� � GNU C++***

� ***MS Visual C++*** ������������� ������ prolog_micro_brain.sln
� ������� ***Free Pascal*** ������������� ������ xpathInduct.dir/xpathInduct.lpr

� ***Linux*** ��� ���������� ����� ���� �������: sh ./install.sh

����� ***tests*** �������� ������� �������. ������� ����� ��������� ����� ���������:
1. ***�� ��������� ������***. ��������, ���� ���������� ��������� ������ ***prog.pro***
�� ������� ��������� ***start***, ����� ���� �������: ./prolog_micro_brain -consult prog.pro start
2. ***�� ��������������***. ��������, ���� ���������� ��������� ������ ***prog.pro***
�� ������� ��������� ***start***, ����� ��������� ���� ./prolog_micro_brain ��� ���������� � � �����������
���� ���� ��� �������:

   consult('prog.pro').

   start.

��� ������ �� �������������� ���������� ���� ������� halt.

***�������������� ��������� � ��������***
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
- is (�������������� + - * / � ������� max, abs, round)
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
