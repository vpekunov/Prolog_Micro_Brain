run :- nload('exprmv2fp.net',NET11),granularize(NET11,NET2),train(NET2,20000,NET22),nsimplify(NET22,NET4,3),write(NET4),nsave(NET22,'exprmv2fp.sparse.net').
