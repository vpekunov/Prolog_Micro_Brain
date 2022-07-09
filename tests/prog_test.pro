	test([H|T]):-write(H),=(T,[11]).

	work(A) :- test(A).

	analyze_expr(L,[],[],[V1],[],[],[],[]):-
		append(D,[id(V1)],L),
		write(D),
		!.

	analyze_expr([id(V1)],[V1],[],[],[],[],[],[]):-
		write(a),
		!.

	analyze_expr(L,INS,OUTS,NEWS,FUNS,PROCS,REFS,Lazies):-
		write(b),
		=(INS,[]),
		=(OUTS,[]),
		=(NEWS,[]),
		=(FUNS,[]),
		=(PROCS,[]),
		=(REFS,[]),
		=(Lazies,[]).

	zero(0,A):-=(A,1),!.
	zero(_,A):-=(A,2),!.

	bypass_idxs(['['|T],T1,Lazies):-
		=(T1,T), =(Lazies,[]),
		!.

	bypass_idxs(L,L,[]):-
		!.

	test_not :- \+ member(1,[2,3,4]), write(V).

	start :- test_not, bypass_idxs(['*',id(v)],V,M), write(V), write(M), nl,
		zero(0,B), write(B), analyze_expr([1,2,3,id(5),4,5],_,_,S,_,_,_,_), write(S), work([1|A]), write(A).
