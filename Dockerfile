FROM registry.docker-cn.com/library/ubuntu:16.04

COPY build/java_policy /etc

RUN apt-get update && apt-get install -y gcc g++ make && \
    echo "init success !"
#useradd -u 12001 compiler && useradd -u 12002 code && useradd -u 12003 spj && usermod -a -G code spj

ADD server /code
WORKDIR /code
RUN make
EXPOSE 8080
ENTRYPOINT /bin/bash