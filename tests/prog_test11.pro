	run :-
		({page_id(A),write(A),fail}*5).

	run_ :-
		(*{page_id(A),write(A)}*5).

	run2 :-
		g_assign('&res',[_,_,_,_,_]),
		{page_id(A), B is A+1, g_assign_nth('&res',B,A), fail}*5, {=0},
		g_read('&res',R),
		write(R).

	run2_ :-
		g_assign('&res',[_,_,_,_,_]),
		*{page_id(A), B is A+1, g_assign_nth('&res',B,A)}*5, {&},
		g_read('&res',R),
		write(R).

	run2__ :-
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

	run7 :- {{for(A, 1, 1000000), fail},{for(B, 1, 1000000), fail},{=0}}{{for(A, 1, 1000000), fail},{for(B, 1, 1000000), fail},{=0}},
		{&}.

	run8 :- {(for(M,1,1000000),fail;true),g_assign('m',5),write('1='),g_read('m',A),write(A)}{g_assign('m',6),write('2='),g_read('m',B),write(B)},
		{&},
		g_read('m',X),
		write(X).

	run9 :- for(A,1,10),{for(M,1,100000000),!,fail},{=0},write(A),fail.

	run10 :- for(A,1,10),{write(A)},fail;{&}.

	run11 :- simples_par(8000,1).

	run12 :- simples_par(8000,2).

	run13 :- simples_par(8000,3).

	run14 :- simples_par(8000,4).

	run15 :- {g_assign('m',5),{g_assign('m',6),g_read('m',A),write(A),fail},{=0},g_read('m',B),write(B)},
		{&},
		g_read('m',X),
		write(X).

	run16 :- =(C,1),{=(B,1),{{{{=(A,2)}}}},(run6;true),{&}},=(D,1),{&},write(A),write(B),write(C),write(D).

	run17 :- g_assign('&A',0), *{g_read('&A',A), write(A), B is A+1, g_assign('&A',B), =(B,5); rollback}, {&}.

	run18 :- retractall(a(_)), !{(for(M,1,1000000),fail;true),asserta(a(1)),write(1)},!{retractall(a(_)),asserta(a(2)),write(2),asserta(s(1))},{&},
		 nl,
		 a(A),
		 write(A),
		 fail;
		 true.

	run19 :- retractall(a(_)), !{(for(M,1,1000),fail;true),asserta(a(1)),write(1)},!{asserta(a(2)),write(2)},{&},
		 nl,
		 a(A),
		 write(A),
		 fail;
		 true.

	run20 :- member(A,[1,2,3,4,5]), { write(A) }, fail; {&}.

	run21 :- *for(I,1,20){write(I),*for(A,1,3){write(x),fail};true}.

	run22 :- *for(I,1,20){write(I)},write(I).

	run23a(1) :- write(a).
	run23a(3) :- fail.
	run23a(4) :- member(_,[1,2]).
	run23a(2) :- write(b).

	run23 :- *run23a(A){write(A),fail}.

	run24a(1) :- fail.

	run24 :- *run24a(A){write(A)}.

	run25 :- member(X,[1,2,3]),write(X),*for(A,1,3){write(x)},fail.

	run26 :- {*member(A,[1,2,3]){{write(A)},{&}},write(x),write(A),fail; true},{&}.

	run27 :- asserta(num(x)),retractall(num(_)),*member(A,[1,2,3,4,5]){asserta(num(A)),A>3},num(X),write(X),fail.

	run28 :- *member(A,[1,2,3]){write(x)},write(A).

	run29 :- g_assign('C', 0), *member(A,[1,2,3]){g_read('C',C),write(C),C1 is C+1,g_assign('C',C1)},write(' '),write(C),fail.

	run30 :- g_assign('C', 0), *repeat{g_read('C',C),C1 is C+1,g_assign('C',C1),write(C),C>4}, write(' '), write(C).

	run31a(0) :- !.
	run31a(X) :- !{ assertz(runer31(X)), X1 is X-1, run31a(X1) }, !{ assertz(runer31(X)), Y1 is X-1, run31a(Y1) }, {&}.

	run31 :- run31a(5),!,runer31(A),write(A),write(' '),fail.

	run32 :- simples_new(1000, 0.85), primes(P), write(P).

	run33 :- simples_miller(400, 1), primes(P), write(P).

	run34 :- simples_miller(400, 0), primes(P), write(P).

	run35 :- {*member(A,[1,2,3]){{write(A)},{&},A>1,!},write(x),write(A),fail; true},{&}.

	run36 :- g_assign('S',[]),*?run23a(A){g_read('S',S),append(S,[A],S1),g_assign('S',S1),fail};g_read('S',S),write(S).

	run37g(A) :- member(A,[1,2,3,4]).
	run37 :- *run37g(A){write(A),fail}.

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
		once(retract(prime(First));true),
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
			set_iteration_star_packet(2),
			*for(M, 2, CSqrtN){
				prime(M),
				First is M*2,
				once(mark_nprimes(First, M, N)),
				fail
			}
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
				*{
					g_read('&schedule', Sch),
					page_id(ID),
					Thread is ID+1,
					nth(Thread, Sch, [From, End]),
					for (K, 2, CSqrtN),
						prime(K),
						R is From - floor(From/K)*K,
						Jp is From - R - K,
						(
							<(Jp,2)->
								J is Jp + 2*K;
								J is Jp + K
						),
						once(mark_nprimes(J, K, End)),
						fail
				},
				fail
			);
			true
		),
		{&},
		!.

% Non-effective, but *predicate{} demonstration with some really parallel iterations!

	simples_new(N,NonParallelism):-
		End is floor(N*NonParallelism),
		Start is End+1,
		retractall(primes(_)),
		asserta(primes([])),
		once(
			for (I, 2, End),
				primes(L),
				once(
					member(Prime, L),
						(R is I - floor(I/Prime)*Prime,
							(
								==(R, 0)->
									(!,fail);
									fail
							)
						);
						(
							append(L, [I], L1),
							retract(primes(_)),
							asserta(primes(L1))
						)
				),
				fail
			;
			true
		),
		once(
			set_iteration_star_packet(3),
			*for (I, Start, N){
				primes(L),
				once(
					member(Prime, L),
						(R is I - floor(I/Prime)*Prime,
							(
								==(R, 0)->
									(!,fail);
									fail
							)
						);
						(
							append(L, [I], L1),
							retract(primes(_)),
							asserta(primes(L1))
						)
				),
				fail
			};
			true
		).

	gcd1(A,A,A).
	gcd1(A,0,A).
	gcd1(0,A,A).
	gcd1(A,B,GCD):-
		A>B, A1 is A-B, gcd1(A1,B,GCD).
	gcd1(A,B,GCD):-
		A<B, B1 is B-A, gcd1(A,B1,GCD).

	gcd(A,B,GCD) :-
		A1 is abs(A),
		B1 is abs(B),
		once(gcd1(A1, B1, GCD)).

	mod_pow(_, 0, 1, 0).
	mod_pow(_, 0, _, 1).
	mod_pow(A, P, N, Result) :-
		P1 is P-1,
		once(mod_pow(A, P1, N, R1)),
		D is floor(A*R1/N),
		R is round(A*R1 - D*N),
		=(Result, R).

	is_power_of_number(N) :-
		N1 is N-1,
		mod_pow(2, N1, N, R),
		R1 is R-1,
		gcd(R1, N, R2),
		R2 < 2.

	is_strong_pp_loop(Exp, Ost_1, I, L) :-
		Exp1 is floor(Exp / 2),
		(
			once(mod_pow(L, Exp1, I, Ost_1))->
				true;
				(
					R is round(Exp1 - 2*floor(Exp1/2)),
					(
						==(R, 0)->
							once(is_strong_pp_loop(Exp1, Ost_1, I, L));
							(
								!, once(mod_pow(L, Exp1, I, 1))
							)
					)
				)
		).

	is_strong_pseudo_prime(I, L) :-
		Exp is I-1,
		Ost_1 is Exp,
		once(mod_pow(L, Exp, I, 1)),
		once(is_strong_pp_loop(Exp, Ost_1, I, L)).

	simples_miller(N,NonParallelism):-
		retractall(primes(_)),
		LogN0 is log(N),
		LogLogN0 is log(LogN0),
		Max0 is floor(LogN0 * LogLogN0 / log(2.0)),
		simples_new(Max0, 1),
		primes(L),
		append(_, [LAST], L),
		append(L, [N], LX),
		Start0 is LAST + 1,
		End is Start0 + floor((N-Start0)*NonParallelism),
		Start is End+1,
		retractall(new_primes(_)),
		asserta(new_primes([])),
		once(
			for (I, Start0, End),
				once(
					LogN is log(I),
					LogLogN is log(LogN),
					Max is floor(LogN * LogLogN / log(2.0)),
					\+ is_power_of_number(I),
					member(Prime, LX),
						(
							once(Prime > Max),
							(
								new_primes(LL),
								append(LL, [I], L1),
								retract(new_primes(_)),
								asserta(new_primes(L1)),
								!
							);
							(
								\+ once(is_strong_pseudo_prime(I, Prime)), !, fail
							)
						)
				),
				fail
			;
			true
		),
		once(
			set_iteration_star_packet(3),
			*for (I, Start, N){
				once(
					LogN is log(I),
					LogLogN is log(LogN),
					Max is floor(LogN * LogLogN / log(2.0)),
					\+ is_power_of_number(I),
					member(Prime, LX),
						(
							once(Prime > Max),
							(
								new_primes(LL),
								append(LL, [I], L1),
								retract(new_primes(_)),
								asserta(new_primes(L1)),
								!
							);
							(
								\+ once(is_strong_pseudo_prime(I, Prime)), !, fail
							)
						)
				),
				fail
			};
			true
		),
		new_primes(Concat),
		append(L, Concat, FOUND),
		retractall(primes(_)),
		retractall(new_primes(_)),
		asserta(primes(FOUND)).
