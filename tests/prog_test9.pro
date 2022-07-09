	start :- once((member(1,[1,2,1]),once(subtract([1,2,3],[1],_)),write(r))),fail.
% Must be one 'r' in output!