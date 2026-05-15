windres my.rc -O coff -o my.res
g++ main.cpp my.res -o app -lsfml-graphics -lsfml-window -lsfml-system -O3 -std=c++23 -I ./SFML-2.5.1/include/ -L ./SFML-2.5.1/lib/