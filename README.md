# Prolog MicroBrain (the simplified analogue of GNU Prolog) Interpreter 

***����� ��������������� ��� MS Visual C++ ��� � GNU C++***

� ***MS Visual C++*** ������������� ������ prolog_micro_brain.sln

� ***GNU C++*** ��� ���������� ����� ���� �������: g++ -o prolog_micro_brain prolog_micro_brain.cpp -O4 -std=c++0x -msse4

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
= ()
- \+
- ;
- =..
- ->
- =
- ==
- <
- >
- \=
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
