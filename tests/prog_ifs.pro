	start:- subtract([1,2,3],[1],[2,3]),
	       write('_'),
               subtract([1,2,3],[2,4,5],X),
               write(X),
               write(0), % write(0), is to print zero!
               ;(;(fail,fail),(write(x),true)),
               (
                =(1,1)->(
% Simply new comment line
                       =(2,2)-> (write(1),
                                 (write(3),fail;
                                  write(4),fail;
                                  (
                                   write(5),
                                   (
                                    fail;
                                    (=(1,3)->write(q);write(8)), !;
                                    write(9)
                                   )
                                  );
                                  write(6)
                                 );
                                 write(2)
                                )
                        )
               ).
