g++ game.cpp -shared -fPIC -o game.so -std=c++11 -I~/include -L~/lib -ldl -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lopengl32 -lglew32

g++ main.cpp -o main -std=c++11 -I~/include -L~/lib -ldl -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lopengl32 -lglew32 -DDLL_FILE=\"game.so\"
