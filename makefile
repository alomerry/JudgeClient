all: clean judge_serve judge_client
judge_serve:judge_serve.cpp
	g++ judge_serve.cpp -o judge_serve -lmysqlclient -L/usr/lib64/mysql -std=c++11
judge_client:judge_client.cpp client.o logger.o configer.o
	g++ judge_client.cpp client.o logger.o configer.o -o judge_client -lmysqlclient -L/usr/lib64/mysql -std=c++11
client.o:client.cpp
	g++ -c client.cpp
logger.o:logger.cpp configer.o
	g++ -c logger.cpp
configer.o:configer.cpp
	g++ -c configer.cpp
clean:
	rm -rf judge_serve judge_client *.o