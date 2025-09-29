FROM ubuntu:24.04

RUN apt-get update && apt-get install -y gcc libpq-dev curl && rm -rf /var/lib/apt/lists/*
# Dipendenze di compilazione+RUN apt-get update && apt-get install -y \
RUN apt-get update && apt-get install -y \
    gcc build-essential libpq-dev curl ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copia i tuoi sorgenti
COPY . .

RUN curl -L https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.c -o mongoose.c && \
    curl -L https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.h -o mongoose.h

# Ensure source files are readable (optional, but helps if permissions are an issue)
RUN chmod +r $(find . -type f -name '*.c') $(find . -type f -name '*.h')

# Compile without sh -lc to avoid shell interpretation issues
RUN gcc -O2 -Wall \
  -I. -I/usr/include/postgresql \
  server.c mongoose.c \
  httpUtils.c \
  controllers/authController.c \
  controllers/lobbyController.c \
  controllers/userController.c  \
  $(find moduli -type f -name '*.c') \
  -lpq -o server

EXPOSE 8080

# Corrected CMD syntax
CMD ["./server"]