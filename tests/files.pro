	dummy(2101, 'remember me').

	reads(S) :- get_char(S, C), \=(C, end_of_file), !, write(C), (peek_char(S, D),write('['),write(D),write(']');true), !, reads(S).
	reads(_).

	read_pro(S):-read_token(S,T), \=(T,punct(end_of_file)), !, write(' '), write(T), read_pro(S).
	read_pro(_).

	start:- open('hello.txt', write, S), write(S), nl, write(S, 'Hello, world!\n'), nl(S), close(S),
		open('hello.txt', read, S1), write(S1), reads(S1), close(S1),
		open('files.pro', read, S3), read_pro(S3), close(S3),
			open('no.txt', read, S2), write(S2).
