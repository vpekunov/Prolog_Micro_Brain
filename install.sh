#!/bin/bash

chmod -R +x *
cd ./xpathInduct.dir
fpc -B -O3 -Mobjfpc -FcUTF-8 -fPIC ./xpathInduct.lpr
cp ./libxpathInduct.so ../
cp ./libxpathInduct.dylib ../libxpathInduct.so
cd ..
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Running on a Mac"
   /opt/homebrew/opt/llvm/bin/clang++ -o prolog_micro_brain values.cpp tinyxml2.cpp elements.cpp prolog_micro_brain.cpp -std=c++17 -O4 -lm -ldl -fpermissive -I/opt/homebrew/include -L/opt/homebrew/lib -lmlx -fopenmp
else
   g++ -o prolog_micro_brain values.cpp tinyxml2.cpp elements.cpp prolog_micro_brain.cpp -std=c++17 -O4 -lm -ldl -fopenmp -fpermissive
fi
