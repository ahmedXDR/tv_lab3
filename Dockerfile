FROM ubuntu:latest
RUN apt-get update && apt-get install -y sysbench python3
COPY benchmark.py /
