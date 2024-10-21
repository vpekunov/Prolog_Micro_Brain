	run :-
		({page_id(A),write(A),fail}*5).

	run2 :-
		g_assign('&res',[_,_,_,_,_]),
		{page_id(A), B is A+1, g_assign_nth('&res',B,A), fail}*5, {=0},
		g_read('&res',R),
		write(R).

	run21 :-
		g_assign('res',[_,_,_,_,_]),
		{page_id(A), B is A+1, g_assign_nth('res',B,A), unset('A'), unset('B')}*5, {&},
		g_read('res',R),
		write(R).

	run3 :-
		for(A, 1, 4000000), fail.

	run4 :-
		{for(A, 1, 1000000), fail}{for(B, 1, 1000000), fail}{for(A, 1, 1000000), fail}{for(B, 1, 1000000), fail},
		{=0}.

	run5 :-
		{nl}*5,
		{&}.

	make_page(0,[]).
	make_page(N,[ID|L]):-
		{page_id(ID0), ID is ID0 + N},
		N1 is N-1,
		make_page(N1,L),
		{&}.

	run6 :- make_page(6,R), write(R).

	run7 :- {{for(A, 1, 1000000), fail},{for(B, 1, 1000000), fail},{&}}{{for(A, 1, 1000000), fail},{for(B, 1, 1000000), fail},{&}},
		{&}.

	run8 :- {(for(M,1,1000),fail;true),g_assign('m',5),g_read('m',A),write(A)}{g_assign('m',6),g_read('m',B),write(B)},
		{&},
		g_read('m',X),
		write(X).

	run9 :- for(A,1,10),{for(M,1,100000000),!,fail},{=0},write(A),fail.

	run10 :- for(A,1,10),{write(A)},fail;{&}.
