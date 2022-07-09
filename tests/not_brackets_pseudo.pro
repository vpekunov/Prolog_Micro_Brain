	goal1:- member(B,[1,2]), write(B), call(member(a,[1,2,3]),!,fail;true);true.
	goal2:- member(B,[1,2]), write(B), call(member(A,[1,2,3]),!,fail;true);true.

	goal3:- member(B,[1,2]), write(B), \+ (member(a,[1,2,3]));true.
	goal4:- member(B,[1,2]), write(B), \+ (member(A,[1,2,3]));true.

	start:-once(goal1),nl,once(goal2),nl,once(goal3),nl,once(goal4).
