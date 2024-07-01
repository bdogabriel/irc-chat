all:
	g++ src/server.cpp -o exe/server -pthread -Wall -Werror
	g++ src/client.cpp -o exe/client -pthread -Wall -Werror

server:
	g++ src/server.cpp -o exe/server -pthread -Wall -Werror

client:
	g++ src/client.cpp -o exe/client -pthread -Wall -Werror

clean:
	rm exe/server exe/client