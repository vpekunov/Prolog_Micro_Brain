has_spawns(_):-
   !.

start:-\+ call(has_spawns(2)), write('+').
start:-write(w).
