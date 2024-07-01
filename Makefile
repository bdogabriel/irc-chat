all: server client

server: server.cpp
	g++ src/server.cpp -o exe/server -pthread -Wall -Werror

client: client.cpp
	g++ src/client.cpp -o exe/client -pthread -Wall -Werror

run-server:
	./exe/server

run-client:
	./exe/client

clean:
	rm exe/server exe/client