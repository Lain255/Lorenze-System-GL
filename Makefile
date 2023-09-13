LIBS	:= -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl
SOURCE	:= main.cpp glad.c
FLAGS	:= -O3 -Wall

run:
	g++ $(FLAGS) $(SOURCE) $(LIBS)
	./a.out
	rm a.out

