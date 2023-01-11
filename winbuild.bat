g++ -g -shared -o game.dll game.cpp -std=c++11 -IC:\Users\Noxide\Desktop\wkspace\include -LC:\Users\Noxide\Desktop\wkspace\lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lsfml-audio -lopengl32 -lglew32 -lwsock32 -lWs2_32 -lfreetype

g++ -g -o main main.cpp -std=c++11 -IC:\Users\Noxide\Desktop\wkspace\include -LC:\Users\Noxide\Desktop\wkspace\lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lsfml-audio -lopengl32 -lglew32 -lwsock32 -lWs2_32 -DDLL_FILE=\"game.dll\"

rem g++ -DCLIENT_PORT=9002 -shared -o game2.dll game.cpp -std=c++11 -IC:\Users\Noxide\Desktop\wkspace\include -LC:\Users\Noxide\Desktop\wkspace\lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lopengl32 -lglew32 -lwsock32 -lWs2_32

rem g++ -o main2 main.cpp -std=c++11 -IC:\Users\Noxide\Desktop\wkspace\include -LC:\Users\Noxide\Desktop\wkspace\lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lopengl32 -lglew32 -lwsock32 -lWs2_32 -DDLL_FILE=\"game2.dll\"
