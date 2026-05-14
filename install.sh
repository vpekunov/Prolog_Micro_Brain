#!/bin/bash

chmod -R +x *
cd ./xpathInduct.dir
fpc -B -O3 -Mobjfpc -FcUTF-8 -fPIC ./xpathInduct.lpr
cp ./libxpathInduct.so ../
cd ..
g++ -o prolog_micro_brain tinyxml2.cpp elements.cpp prolog_micro_brain.cpp -std=c++11 -O4 -lm -lboost_system -lboost_filesystem -ldl -I/opt/homebrew/include -L/opt/homebrew/lib
if [ $? -ne 0 ]; then
   g++ -o prolog_micro_brain tinyxml2.cpp elements.cpp prolog_micro_brain.cpp -std=c++11 -O4 -lm -lboost_filesystem -ldl -I/opt/homebrew/include -L/opt/homebrew/lib
fi
g++ -c tinyxml2.cpp elements.cpp prolog_micro_brain.cpp -std=c++11 -O4 -lm -lboost_system -lboost_filesystem -ldl -fPIC -I/opt/homebrew/include -L/opt/homebrew/lib
if [ $? -ne 0 ]; then
   g++ -c tinyxml2.cpp elements.cpp prolog_micro_brain.cpp -std=c++11 -O4 -lm -lboost_filesystem -ldl -fPIC -I/opt/homebrew/include -L/opt/homebrew/lib
fi
