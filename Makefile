all: server client

server: ./src/server.cpp
	g++ ./src/server.cpp -o ./exe/server -pthread -Wall -Werror

client: ./src/client.cpp
	g++ ./src/client.cpp -o ./exe/client -pthread -Wall -Werror

run-server:
	./exe/server

run-client:
	./exe/client

clean:
	rm exe/server exe/client