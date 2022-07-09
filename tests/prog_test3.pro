	p1:-write(1),!.
	p1:-write(2).

	p2:-member(A,[1,2]),p1,write(A),fail.

	p3:-write(4).
	p3:-write(5).

	make_once:-call(p3),!.

	p4:-make_once,fail.

	p45:-once(p3),fail.

	p5:-write(1),call(!).
	p5:-write(2).

	p6:-p5,fail.

	p7:-call(p5),fail.

	p8:-write(1).
	p8:-write(2).

	p9:-member(A,[1,2]),once(p8),write(A),fail.

	loop(N,N):-!.
	loop(X,N):-write(X),inc(X,X1),loop(X1,N).

	p10:-member(A,[1,2]),loop(0,5),member(B,[1,2]),!,write(A),write(B).

	p11:-member(X,[1,2,3]),p10,write(X).

	y(1).
	y(0):-p(0).
	p(0):-y(1).
	p(2):-member(A,[1,2]),y(0),!,write(A),fail.
	p(2):-write(3).

	test:-p(2),fail.

a(a0).
a(a1).
a(a2).
b(b0).
b(b1).
b(b2).
c(c0).
c(c1).
c(c2).

% parentheses added for clarity below

t0(A,B)   :- a(A),!,b(B).
t0(x,x).

t1(A,B)   :- (a(A),!,fail);b(B).
t1(x,x).

t2(A,B,C) :- (a(A) -> (b(B),!)) ; c(C).
t2(x,x,x).

t3(A,B)   :- call(((a(A),!,fail);b(B))).
t3(x,x).

t4(A)     :- \+ (a(A),!,fail).     % there must be a space after \+

d1(A,B)   :- call((a(A),!,b(B))).  % ! is part of the called goal

d3(X)     :- a(X);(b(X),!);c(X).   % actually 3 clauses, one of which is d3(X) :- b(X),!,true.
