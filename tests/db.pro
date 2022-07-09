put_a(Start, Start):-!.
put_a(Start, End):-assert(a(Start)),inc(Start,Next), put_a(Next, End).

start:-consistency(['a']),put_a(12,15),!,assert(a(1)),assert(b(3)),assert(a(4)),assert(b(6)),assert(a(8)),a(X),write(X),write(' '),fail.
