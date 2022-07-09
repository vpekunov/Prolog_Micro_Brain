	start:- subtract([1,2,3],[1],[2,3]),
	       write('_'),
               subtract([1,2,3],[2,4,5],X),
               write(X),
               write(0),
               (
                true, (
                       write(1), (
                                  write(3),fail;
                                  write(4),fail;
                                  (
                                   write(5),
                                   (
                                    fail;
                                    write(8), !;
                                    write(9)
                                   )
                                  );
                                  write(6)
                                 );
                       write(2)
                      )
               ).
