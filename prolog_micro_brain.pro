	send :- current_predicate(A), listing(A), fail.
	send :- write("Ok").
	mars_send(FName) :- mars(on), send, mars(FName).
	mars_recv(FName) :- mars_decode(FName).
