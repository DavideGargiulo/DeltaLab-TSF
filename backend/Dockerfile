FROM gcc:13
WORKDIR /app
COPY . .
RUN gcc -o server $(find . -name "*.c") -lpthread
CMD ["./server"]