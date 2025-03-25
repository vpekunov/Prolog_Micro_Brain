% ��� ��������� ����������� ����� � ��������� MIN..MAX ���� ������� �������� generate(MIN,MAX,V). ��� ���� � V ������� ������ �����������
% �����. �� ��������� -- ������ ����, ������� �� ������������� ����� MAX > 1000.
% �������� -- �������� �� V ����������� ������, ����������� ���������� check(V)


% ������� ��������� ������ ��������� -- ��� �� ����� ��������� �������� ��������� �����
get_dividers(V, A, []):-
  A2 is floor(V/2),
  >(A, A2),
  !.

% �������, ��������������� ������, ����� A -- �������� V
get_dividers(V, A, [A|T]):-
  0 is V - A*floor(V/A), % ���� V ������� �� A ��� �������
  !,
  NEXT is A+1,
  get_dividers(V, NEXT, T),
  !.

% �������, ��������������� ������, ����� A -- �� �������� V
get_dividers(V, A, L):-
  NEXT is A+1,
  get_dividers(V, NEXT, L),
  !.

% ����� ������� ������ ����� ����
summate([], 0):-
  !.

% ����� �� ������� ������ �� ������ H � ������ T. ��������� �������� � S.
summate([H|T], S):-
  summate(T, S1), % ������� ����� ��������� ����� ������
  S is H+S1, % � ���������� �� � ������ ���������
  !.

% �������� -- �������� �� V ����������� ������
check(V):-
  get_dividers(V, 1, Divs), % ���������� ��� ��������
  summate(Divs,V). % ���������, ��� ����� ��������� ����� �����

generate(MIN,MAX,L):-
  g_assign('L',[]),
  once(
   *for(A,MIN,MAX){
     once(check(A)), % ��������� ������� �����
     g_read('L',L0),
     append(L0, [A], L1),
     g_assign('L', L1),
     fail
   };
   g_read('L',L)
  ).

generate_1(MIN,MAX,L):-
  g_assign('L',[]),
  once(
   for(A,MIN,MAX),
     once(check(A)), % ��������� ������� �����
     g_read('L',L0),
     append(L0, [A], L1),
     g_assign('L', L1),
     fail;
   g_read('L',L)
  ).

run1:-
  generate_1(300, 600, L), write(L).

run2:-
  generate(300, 600, L), write(L).
