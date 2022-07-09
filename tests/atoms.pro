	p1 :- atom_concat(A,B,'remember'), write('['),write(A),write('+'),write(B),write(']'),nl,fail.
	p1.
	p2 :- atom_chars('try it', L), write(L), write(' - '), atom_chars(A, L), write(A), nl.
	p3 :- =(A,11), number_atom(A,X), write(X), number_atom(N,X), write('  '), write(N), nl.
	start :- p1, p2, p3, write('\'\'\n'), nl,
		(
		 (number(11),number(12.5))->(write(11),write(' '),write(12.5), write(' are numbers!'))
		),
		(
		 number(e1)->write('Error!');write('Correct!')
		).