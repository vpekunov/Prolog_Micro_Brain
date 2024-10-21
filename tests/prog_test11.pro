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

	run11 :- simples_par(8000,1).

	run12 :- simples_par(8000,2).

	run13 :- simples_par(8000,3).

	run14 :- simples_par(8000,4).

	generate_facts(This,N):-
		>(This,N),
		!.
	generate_facts(This,N):-
		T1 is This+1,
		assertz(prime(This)),
		generate_facts(T1,N).

	bisect(A,B,N,X):-
	      <(A,B)->
	       (
		C is floor((A+B)/2),
		V is C*C,
		C1 is C+1,
		V1 is C1*C1,
		(
		 (=(V,N);(<(V,N),>(V1,N)))->
		   =(X,C);
		   (
		     <(V,N)->
			(
			  A1 is C+1,
			  bisect(A1,B,N,X)
			);
			(
			  B1 is C-1,
			  bisect(A,B1,N,X)
			)
		   )
		)
	       );
	       =(X,B).

	sqrt(N,X):-
		once(
			bisect(0,N,N,X),
			!
		).

	mark_nprimes(First, _, N):-
		>(First, N).
	mark_nprimes(First, Step, N):-
		once((retract(prime(First));true)),
		Next is First + Step,
		once(mark_nprimes(Next, Step, N)).

	simples1(N) :-
		sqrt(N, SqrtN),
		SN2 is SqrtN*SqrtN,
		(
			<(SN2, N)->
				CSqrtN is SqrtN+1;
				=(CSqrtN, SqrtN)
		),
		retractall(prime(_)),
		generate_facts(2,N),
		(
		  (
			for(M, 2, CSqrtN),
				prime(M),
				First is M*2,
				once(mark_nprimes(First, M, N)),
				fail
		  );
		  true
		),
		!.
		
	simples_par(N,M) :-
		sqrt(N, SqrtN),
		SN2 is SqrtN*SqrtN,
		(
			<(SN2, N)->
				CSqrtN is SqrtN+1;
				=(CSqrtN, SqrtN)
		),
		simples1(CSqrtN),
		!,
		P is CSqrtN+1,
		generate_facts(P,N),
		!,
		Chunk is floor((N - CSqrtN + 1) / M),
		Rest is (N - CSqrtN + 1) - Chunk*M,
		M1 is M-1,
		g_assign('&schedule',[]),
		g_assign('&from',CSqrtN),
		(
			(
			  for (I, 0, M1),
				g_read('&from',From),
				(
					>(Rest, I)->
						Length is Chunk+1;
						=(Length, Chunk)
				),
				End is From+Length-1,
				g_read('&schedule', Sch),
				g_assign('&schedule', [[From,End]|Sch]),
				Next is End+1,
				g_assign('&from', Next),
				fail
			);
			true
		),
		(
			(
			  for (I, 0, M1),
				{
					g_read('&schedule', Sch),
					page_id(ID),
					Thread is ID+1,
					nth(Thread, Sch, [From, End]),
					for (K, 2, CSqrtN),
						prime(K),
						R0 is From - ceiling(From/K)*K,
						(
							=(From, K)->
								=(R,K);
								(
									\=(R0,0)->
										(R is K - R0);
										=(R, R0)
								)
						),
						J0 is From + R - K,
						(
							<(J0,0)->
								(J is J0+K);
								=(J, J0)
						)
						once(mark_nprimes(J, K, End)),
						fail
				},
				fail
			);
			true
		),
		{=0},
		!.
