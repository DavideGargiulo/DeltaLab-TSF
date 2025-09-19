FROM ubuntu:24.04

# Dipendenze di compilazione
RUN apt-get update && apt-get install -y gcc libpq-dev curl && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copia i tuoi sorgenti
COPY . .

RUN curl -L https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.c -o mongoose.c && \
    curl -L https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.h -o mongoose.h

RUN sh -lc "gcc -O2 -Wall -I. -I/usr/include/postgresql \
    server.c mongoose.c \$(find moduli -type f -name '*.c') \
    -lpq -o server"

CMD ["./server"]
