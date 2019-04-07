all: clean Client Alarm #Test
Test:
	g++ -o test test.cc -std=c++11
Alarm:
	g++ -o alarm alarm.cc -std=c++11
Client:
	g++ -o client judge_client.cpp -lmysqlclient -L/usr/lib64/mysql -std=c++11
clean:
	rm -rf test alarm client