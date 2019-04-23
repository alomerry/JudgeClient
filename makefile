all: clean Serve Client Alarm Test Main
Main:
	g++ -o ./test/main ./test/main.cpp -std=c++11
Test:
	g++ -o ./test/test ./test/test.cc -std=c++11
Alarm:
	g++ -o ./test/alarm ./test/alarm.cc -std=c++11
Serve:
	g++ -o serve judge_serve.cpp -lmysqlclient -L/usr/lib64/mysql -std=c++11
Client:
	g++ -o client judge_client.cpp -lmysqlclient -L/usr/lib64/mysql -std=c++11
clean:
	rm -rf ./test/alarm client serve ./test/test ./test/main