FROM ubuntu:24.04
RUN apt-get update && apt-get install -y gcc libpq-dev
WORKDIR /app
COPY . .
RUN gcc -I/usr/include/postgresql server.c moduli/dbConnection.c -lpq -o server
CMD ["./server"]