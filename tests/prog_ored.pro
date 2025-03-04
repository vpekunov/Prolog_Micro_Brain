	variants(L):-append(A,B,L),write(A),write(B),fail.
	variants(_).

	run(X):-member(V,[1,3,X,7]),write(V),fail;true.

	run_once(X):-once(member(V,[1,3,X,7])),write(V),fail;true.

	loop(N,N).
	loop(X,N):-write(X),X1 is X+1,loop(X1,N).

	once1:-once(repeat), write('!'), fail.
	once1.

	once_2:-once(repeat), write('@'), once1, fail.
	once_2.

	handle:-once(once_2),=(A,1),loop(0,5),=(B,2),run(5),run(8),run_once(9),=(B,2),=([A,B],[_,V]),write(V),variants([3,5,2]),
		append([1,2|[3,4,5]],[7,8],P),write(P).

	start:-handle.
