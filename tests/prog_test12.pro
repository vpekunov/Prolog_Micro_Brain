% Для генерации совершенных чисел в диапазоне MIN..MAX надо вызвать предикат generate(MIN,MAX,V). При этом в V попадет список совершенных
% чисел. Их генерация -- долгое дело, поэтому не рекомендуется брать MAX > 1000.
% Проверка -- является ли V совершенным числом, реализуется предикатом check(V)


% Условие остановки поиска делителей -- они не могут превышать половину исходного числа
get_dividers(V, A, []):-
  A2 is floor(V/2),
  >(A, A2),
  !.

% Правило, соответствующее случаю, когда A -- делитель V
get_dividers(V, A, [A|T]):-
  0 is V - A*floor(V/A), % Если V делится на A без остатка
  !,
  NEXT is A+1,
  get_dividers(V, NEXT, T),
  !.

% Правило, соответствующее случаю, когда A -- не делитель V
get_dividers(V, A, L):-
  NEXT is A+1,
  get_dividers(V, NEXT, L),
  !.

% Сумма пустого списка равна нулю
summate([], 0):-
  !.

% Сумма не пустого списка из головы H и хвоста T. Результат помещаем в S.
summate([H|T], S):-
  summate(T, S1), % Считаем сумму хвостовой части списка
  S is H+S1, % и складываем ее с первым элементом
  !.

% Проверка -- является ли V совершенным числом
check(V):-
  get_dividers(V, 1, Divs), % Генерируем все делители
  summate(Divs,V). % Проверяем, что сумма делителей равна числу

generate(MIN,MAX,L):-
  g_assign('L',[]),
  once(
   *for(A,MIN,MAX){
     once(check(A)), % Проверяем текущее число
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
     once(check(A)), % Проверяем текущее число
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
