% Определяем родные языки для каждого туриста
native(lvengr,vengr).
native(lpolyak,polyak).
native(lfinn,finn).
native(lshved,shved).
native(lnemets,nemets).

% paired(A,B) -- проверяем, могут ли A и B общаться хоть на каком-нибудь языке
paired(person(P1,_),person(_,L2)):- % Может ли B общаться на родном для A языке
  native(L,P1),
  member(L,L2).
paired(person(_,L1),person(P2,_)):- % Может ли A общаться на родном для B языке
  native(L,P2),
  member(L,L1).
paired(person(_,L1),person(_,L2)):- % Может быть A и B знают общий неродной язык?
  member(L,L1),
  member(L,L2).

% sum(L,S) -- принимаем список L "персон-языков" и считаем, сколько языков они знают в сумме -- помещаем в S
sum([],0). % Сумма пустого списка равна нулю
sum([person(_,L1)|T],N):-
  length(L1,N1),
  sum(T,NT),
  N is NT + N1.

% countl(L,LANG,N) -- принимаем список Lб каждый элемент которого = список языков туриста. Подсчитываем, сколько раз язык LANG
% содержится в этих списках = суммируем и помещаем в N.
countl([],_,0). % В пустом списке никаких языков нет.
countl([L|T],Lang,N):- % Если язык Lang есть в текущем подсписке -- добавляем к результату единицу
  member(Lang,L),
  countl(T,Lang,NN),
  N is NN + 1.
countl([L|T],Lang,N):- % Если язык Lang отсутствует в текущем подсписке -- ничего в результат не добавляем
  \+ member(Lang,L),
  countl(T,Lang,N).

% intersectl(A,B,C) -- пересекаем списки A и B как множества, результат помещаем в C.
intersectl([],_,[]). % Пустой список пересекаем с любым = получаем пустой список
intersectl(_,[],[]). % Любой список пересекаем с пустым = получаем пустой список
intersectl([H|T],L,[H|T1]):- % Элемент первого списка есть по втором, значит добавляем его в результат
  member(H,L),
  intersectl(T,L,T1).
intersectl([H|T],L,T1):- % Элемент первого списка отсутствует во втором -- пропускаем его.
  \+ member(H,L),
  intersectl(T,L,T1).

% variant(L,V) -- Это генерация варианта. Предикат дает множество решений, каждый его вызов дает одно решение.
% Каждое решение (вариант) V = подмножество входного списка L.
% Например, если L=[lfinn,lshved], то будет четыре варианта решения: [],[lfinn],[lshved],[lfinn,lshved].
variant(_,[]). % Всегда есть вариант -- пустое множество.
variant(Langs,[L|T]):-
  append(_,[L|Right],Langs), % Генерируем варианты из входного списка -- здесь и появляется множество решений.
  variant(Right,T).

% Генерируем вариант и проверяем его размер. По условию, каждый турист знает хотя бы один язык, но и не более трех (не считая родного)
% (иначе все спокойно говорили бы на одном общем языке)
pvariant(Langs,SubLangs):-
  variant(Langs,SubLangs),
  length(SubLangs,N),
  N > 0, % Хотя бы один язык!
  N < 4. % Но и не более 4 языков.

% Решаем! Результат = список LL
solve1(LL):-
  % Генерируем вариант (родные языки для каждого туриста НЕ ВКЛЮЧАЕМ)
  pvariant([lpolyak,lfinn,lshved,lnemets],Lvengr),
  pvariant([lvengr,lfinn,lshved,lnemets],Lpolyak),
  pvariant([lvengr,lpolyak,lshved,lnemets],Lfinn),
  pvariant([lvengr,lpolyak,lfinn,lnemets],Lshved),
  pvariant([lvengr,lpolyak,lfinn,lshved],Lnemets),
  % Формируем текущий вариант списка "Турист-языки"
  =(LL, [person(vengr,Lvengr), person(polyak,Lpolyak), person(finn,Lfinn), person(shved,Lshved), person(nemets,Lnemets)]),
  % Проверяем среднее арифметическое. Если оно = 2 (языка), то общее количество известных языков = 5*2
  once(sum(LL,NN)), % Здесь и далее используем once, чтобы получать при вызове единственный ответ, без пересогласований
  NN is 5*2,
  % Проверяем ситуацию уплывания шведа. Все оставшиеся знают общий язык (для кого-то родной).
  member(A,[lvengr|Lvengr]),
  \+ =(A,lnemets), % Общий язык поляка+финна, а значит и поляка+венгра+немца+финна -- не немецкий.
  member(A,[lpolyak|Lpolyak]),member(A,[lfinn|Lfinn]),member(A,[lnemets|Lnemets]),
  % Проверяем ситуацию уплывания финна. Все оставшиеся знают общий язык (для кого-то родной).
  member(B,[lvengr|Lvengr]),member(B,[lpolyak|Lpolyak]),member(B,[lshved|Lshved]),member(B,[lnemets|Lnemets]),
  % Поляк и финн знают два общих языка
  once(intersectl(Lpolyak,Lfinn,LPF)),length(LPF,2),
  % Венгр и швед знают один общий язык
  once(intersectl(Lvengr,Lshved,LVS)),length(LVS,1),
  % Венгр и поляк знают по три языка
  length(Lvengr,3),length(Lpolyak,3),
  % Для каждой пары туристов найдется общий язык
  call((member(P1,LL),member(P2,LL),\+ once(paired(P1,P2)), !, fail; true)),
  % Во всей кампании по-фински и по-польски говорят всего двое (видимо, включая родные языки). По-шведски говорят двое + сам швед.
  =(V, [[lvengr|Lvengr], [lpolyak|Lpolyak], [lfinn|Lfinn], [lshved|Lshved], [lnemets|Lnemets]]),
  once(countl(V,lfinn,2)),
  once(countl(V,lpolyak,2)),
  once(countl(V,lshved,3)).

run1:-
  solve1(LL), write(LL), fail.

% Решаем! Результат = solution(_)
solveN:-
  % Генерируем вариант (родные языки для каждого туриста НЕ ВКЛЮЧАЕМ)
  retractall(solution(_)),
  pvariant([lpolyak,lfinn,lshved,lnemets],Lvengr),
  *{
     pvariant([lvengr,lfinn,lshved,lnemets],Lpolyak),
     pvariant([lvengr,lpolyak,lshved,lnemets],Lfinn),
     pvariant([lvengr,lpolyak,lfinn,lnemets],Lshved),
     pvariant([lvengr,lpolyak,lfinn,lshved],Lnemets),
     % Формируем текущий вариант списка "Турист-языки"
     =(LL, [person(vengr,Lvengr), person(polyak,Lpolyak), person(finn,Lfinn), person(shved,Lshved), person(nemets,Lnemets)]),
     % Проверяем среднее арифметическое. Если оно = 2 (языка), то общее количество известных языков = 5*2
     once(sum(LL,NN)), % Здесь и далее используем once, чтобы получать при вызове единственный ответ, без пересогласований
     NN is 5*2,
     % Проверяем ситуацию уплывания шведа. Все оставшиеся знают общий язык (для кого-то родной).
     member(A,[lvengr|Lvengr]),
     \+ =(A,lnemets), % Общий язык поляка+финна, а значит и поляка+венгра+немца+финна -- не немецкий.
     member(A,[lpolyak|Lpolyak]),member(A,[lfinn|Lfinn]),member(A,[lnemets|Lnemets]),
     % Проверяем ситуацию уплывания финна. Все оставшиеся знают общий язык (для кого-то родной).
     member(B,[lvengr|Lvengr]),member(B,[lpolyak|Lpolyak]),member(B,[lshved|Lshved]),member(B,[lnemets|Lnemets]),
     % Поляк и финн знают два общих языка
     once(intersectl(Lpolyak,Lfinn,LPF)),length(LPF,2),
     % Венгр и швед знают один общий язык
     once(intersectl(Lvengr,Lshved,LVS)),length(LVS,1),
     % Венгр и поляк знают по три языка
     length(Lvengr,3),length(Lpolyak,3),
     % Для каждой пары туристов найдется общий язык
     call((member(P1,LL),member(P2,LL),\+ once(paired(P1,P2)), !, fail; true)),
     % Во всей кампании по-фински и по-польски говорят всего двое (видимо, включая родные языки). По-шведски говорят двое + сам швед.
     =(V, [[lvengr|Lvengr], [lpolyak|Lpolyak], [lfinn|Lfinn], [lshved|Lshved], [lnemets|Lnemets]]),
     once(countl(V,lfinn,2)),
     once(countl(V,lpolyak,2)),
     once(countl(V,lshved,3)),
     asserta(solution(LL))
  },
  fail;
  {=0}.

runN:-
  solveN, solution(LL), write(LL), fail.
