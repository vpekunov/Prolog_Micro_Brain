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
