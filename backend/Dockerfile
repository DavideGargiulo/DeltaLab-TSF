# FROM gcc:13
# WORKDIR /app
# COPY . .
# RUN gcc -o server $(find . -name "*.c") -lpthread
# CMD ["./server"]

FROM alpine:3.14
RUN apk add --no-cache gcc musl-dev postgresql-dev
WORKDIR /app
COPY . .
RUN gcc -o server $(find . -name "*.c") -lpthread
CMD ["./server"]