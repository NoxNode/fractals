g++ game.cpp -o game.so --std=c++11 -Wall -g -framework OpenGL -L /Library/Frameworks/ -lsfml-window -lsfml-graphics -lsfml-audio -lsfml-system -lsfml-network -lglew -I/Users/wyatt/Documents/projects/opengl/freetype-2.10.0/include -dynamiclib -lfreetype -flat_namespace -DDLL_FILE=\"game.so\"
g++ main.cpp -o main --std=c++11 -Wall -g -framework OpenGL -L /Library/Frameworks/ -lsfml-window -lsfml-graphics -lsfml-audio -lsfml-system -lsfml-network -lglew -DDLL_FILE=\"game.so\"