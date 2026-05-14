#!/bin/bash

chmod -R +x *
cd ./xpathInduct.dir
fpc -B -O3 -Mobjfpc -FcUTF-8 -fPIC ./xpathInduct.lpr
cp ./libxpathInduct.so ../
cd ..
if [[ "$OSTYPE" == "darwin"* ]]; then
   g++ -o prolog_micro_brain tinyxml2.cpp elements.cpp prolog_micro_brain.cpp -std=c++17 -O4 -lm -lboost_system -lboost_filesystem -ldl -I/opt/homebrew/include -L/opt/homebrew/lib -fpermissive
   if [ $? -ne 0 ]; then
      g++ -o prolog_micro_brain tinyxml2.cpp elements.cpp prolog_micro_brain.cpp -std=c++17 -O4 -lm -lboost_filesystem -ldl -I/opt/homebrew/include -L/opt/homebrew/lib -fpermissive
   fi
else
   g++ -o prolog_micro_brain tinyxml2.cpp elements.cpp prolog_micro_brain.cpp -std=c++17 -O4 -lm -lboost_system -lboost_filesystem -ldl -fopenmp -fpermissive
   if [ $? -ne 0 ]; then
      g++ -o prolog_micro_brain tinyxml2.cpp elements.cpp prolog_micro_brain.cpp -std=c++17 -O4 -lm -lboost_filesystem -ldl -fopenmp -fpermissive
   fi
fi
