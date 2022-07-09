   test(V,INS1,INSQ):-
   (
    =(V,[Single])->
      subtract(INS1,[Single],INSQ);
      =(INSQ,INS1)
   ),
   !.

   start:-test([0],[1],K),write(K),fail.
