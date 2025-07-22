FROM gcc:13
WORKDIR /app
COPY . .
RUN gcc -o server server.c -lpthread
CMD ["./server"]