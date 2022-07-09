	loop(0,[[0]]):-!.
	loop(A,[P|T]):-
		=(P,[A]),
		A1 is A-1,
		loop(A1,T).

	start:-loop(3,R),write(R).
