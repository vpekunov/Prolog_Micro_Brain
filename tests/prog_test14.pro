% ���������� ������ ����� ��� ������� �������
native(lvengr,vengr).
native(lpolyak,polyak).
native(lfinn,finn).
native(lshved,shved).
native(lnemets,nemets).

% paired(A,B) -- ���������, ����� �� A � B �������� ���� �� �����-������ �����
paired(person(P1,_),person(_,L2)):- % ����� �� B �������� �� ������ ��� A �����
  native(L,P1),
  member(L,L2).
paired(person(_,L1),person(P2,_)):- % ����� �� A �������� �� ������ ��� B �����
  native(L,P2),
  member(L,L1).
paired(person(_,L1),person(_,L2)):- % ����� ���� A � B ����� ����� �������� ����?
  member(L,L1),
  member(L,L2).

% sum(L,S) -- ��������� ������ L "������-������" � �������, ������� ������ ��� ����� � ����� -- �������� � S
sum([],0). % ����� ������� ������ ����� ����
sum([person(_,L1)|T],N):-
  length(L1,N1),
  sum(T,NT),
  N is NT + N1.

% countl(L,LANG,N) -- ��������� ������ L� ������ ������� �������� = ������ ������ �������. ������������, ������� ��� ���� LANG
% ���������� � ���� ������� = ��������� � �������� � N.
countl([],_,0). % � ������ ������ ������� ������ ���.
countl([L|T],Lang,N):- % ���� ���� Lang ���� � ������� ��������� -- ��������� � ���������� �������
  member(Lang,L),
  countl(T,Lang,NN),
  N is NN + 1.
countl([L|T],Lang,N):- % ���� ���� Lang ����������� � ������� ��������� -- ������ � ��������� �� ���������
  \+ member(Lang,L),
  countl(T,Lang,N).

% intersectl(A,B,C) -- ���������� ������ A � B ��� ���������, ��������� �������� � C.
intersectl([],_,[]). % ������ ������ ���������� � ����� = �������� ������ ������
intersectl(_,[],[]). % ����� ������ ���������� � ������ = �������� ������ ������
intersectl([H|T],L,[H|T1]):- % ������� ������� ������ ���� �� ������, ������ ��������� ��� � ���������
  member(H,L),
  intersectl(T,L,T1).
intersectl([H|T],L,T1):- % ������� ������� ������ ����������� �� ������ -- ���������� ���.
  \+ member(H,L),
  intersectl(T,L,T1).

% variant(L,V) -- ��� ��������� ��������. �������� ���� ��������� �������, ������ ��� ����� ���� ���� �������.
% ������ ������� (�������) V = ������������ �������� ������ L.
% ��������, ���� L=[lfinn,lshved], �� ����� ������ �������� �������: [],[lfinn],[lshved],[lfinn,lshved].
variant(_,[]). % ������ ���� ������� -- ������ ���������.
variant(Langs,[L|T]):-
  append(_,[L|Right],Langs), % ���������� �������� �� �������� ������ -- ����� � ���������� ��������� �������.
  variant(Right,T).

% ���������� ������� � ��������� ��� ������. �� �������, ������ ������ ����� ���� �� ���� ����, �� � �� ����� ���� (�� ������ �������)
% (����� ��� �������� �������� �� �� ����� ����� �����)
pvariant(Langs,SubLangs):-
  variant(Langs,SubLangs),
  length(SubLangs,N),
  N > 0, % ���� �� ���� ����!
  N < 4. % �� � �� ����� 4 ������.

% ������! ��������� = ������ LL
solve1(LL):-
  % ���������� ������� (������ ����� ��� ������� ������� �� ��������)
  pvariant([lpolyak,lfinn,lshved,lnemets],Lvengr),
  pvariant([lvengr,lfinn,lshved,lnemets],Lpolyak),
  pvariant([lvengr,lpolyak,lshved,lnemets],Lfinn),
  pvariant([lvengr,lpolyak,lfinn,lnemets],Lshved),
  pvariant([lvengr,lpolyak,lfinn,lshved],Lnemets),
  % ��������� ������� ������� ������ "������-�����"
  =(LL, [person(vengr,Lvengr), person(polyak,Lpolyak), person(finn,Lfinn), person(shved,Lshved), person(nemets,Lnemets)]),
  % ��������� ������� ��������������. ���� ��� = 2 (�����), �� ����� ���������� ��������� ������ = 5*2
  once(sum(LL,NN)), % ����� � ����� ���������� once, ����� �������� ��� ������ ������������ �����, ��� ����������������
  NN is 5*2,
  % ��������� �������� ��������� �����. ��� ���������� ����� ����� ���� (��� ����-�� ������).
  member(A,[lvengr|Lvengr]),
  \+ =(A,lnemets), % ����� ���� ������+�����, � ������ � ������+������+�����+����� -- �� ��������.
  member(A,[lpolyak|Lpolyak]),member(A,[lfinn|Lfinn]),member(A,[lnemets|Lnemets]),
  % ��������� �������� ��������� �����. ��� ���������� ����� ����� ���� (��� ����-�� ������).
  member(B,[lvengr|Lvengr]),member(B,[lpolyak|Lpolyak]),member(B,[lshved|Lshved]),member(B,[lnemets|Lnemets]),
  % ����� � ���� ����� ��� ����� �����
  once(intersectl(Lpolyak,Lfinn,LPF)),length(LPF,2),
  % ����� � ���� ����� ���� ����� ����
  once(intersectl(Lvengr,Lshved,LVS)),length(LVS,1),
  % ����� � ����� ����� �� ��� �����
  length(Lvengr,3),length(Lpolyak,3),
  % ��� ������ ���� �������� �������� ����� ����
  call((member(P1,LL),member(P2,LL),\+ once(paired(P1,P2)), !, fail; true)),
  % �� ���� �������� ��-������ � ��-������� ������� ����� ���� (������, ������� ������ �����). ��-������� ������� ���� + ��� ����.
  =(V, [[lvengr|Lvengr], [lpolyak|Lpolyak], [lfinn|Lfinn], [lshved|Lshved], [lnemets|Lnemets]]),
  once(countl(V,lfinn,2)),
  once(countl(V,lpolyak,2)),
  once(countl(V,lshved,3)).

run1:-
  solve1(LL), write(LL), fail.

% ������! ��������� = solution(_)
solveN:-
  % ���������� ������� (������ ����� ��� ������� ������� �� ��������)
  retractall(solution(_)),
  pvariant([lpolyak,lfinn,lshved,lnemets],Lvengr),
  *{
     pvariant([lvengr,lfinn,lshved,lnemets],Lpolyak),
     pvariant([lvengr,lpolyak,lshved,lnemets],Lfinn),
     pvariant([lvengr,lpolyak,lfinn,lnemets],Lshved),
     pvariant([lvengr,lpolyak,lfinn,lshved],Lnemets),
     % ��������� ������� ������� ������ "������-�����"
     =(LL, [person(vengr,Lvengr), person(polyak,Lpolyak), person(finn,Lfinn), person(shved,Lshved), person(nemets,Lnemets)]),
     % ��������� ������� ��������������. ���� ��� = 2 (�����), �� ����� ���������� ��������� ������ = 5*2
     once(sum(LL,NN)), % ����� � ����� ���������� once, ����� �������� ��� ������ ������������ �����, ��� ����������������
     NN is 5*2,
     % ��������� �������� ��������� �����. ��� ���������� ����� ����� ���� (��� ����-�� ������).
     member(A,[lvengr|Lvengr]),
     \+ =(A,lnemets), % ����� ���� ������+�����, � ������ � ������+������+�����+����� -- �� ��������.
     member(A,[lpolyak|Lpolyak]),member(A,[lfinn|Lfinn]),member(A,[lnemets|Lnemets]),
     % ��������� �������� ��������� �����. ��� ���������� ����� ����� ���� (��� ����-�� ������).
     member(B,[lvengr|Lvengr]),member(B,[lpolyak|Lpolyak]),member(B,[lshved|Lshved]),member(B,[lnemets|Lnemets]),
     % ����� � ���� ����� ��� ����� �����
     once(intersectl(Lpolyak,Lfinn,LPF)),length(LPF,2),
     % ����� � ���� ����� ���� ����� ����
     once(intersectl(Lvengr,Lshved,LVS)),length(LVS,1),
     % ����� � ����� ����� �� ��� �����
     length(Lvengr,3),length(Lpolyak,3),
     % ��� ������ ���� �������� �������� ����� ����
     call((member(P1,LL),member(P2,LL),\+ once(paired(P1,P2)), !, fail; true)),
     % �� ���� �������� ��-������ � ��-������� ������� ����� ���� (������, ������� ������ �����). ��-������� ������� ���� + ��� ����.
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
