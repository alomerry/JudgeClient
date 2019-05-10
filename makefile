all: clean Serve Client
Serve:
	g++ -o serve judge_serve.cpp -lmysqlclient -L/usr/lib64/mysql -std=c++11
Client:
	g++ -o client judge_client.cpp -lmysqlclient -L/usr/lib64/mysql -std=c++11
clean:
	rm -rf client serve