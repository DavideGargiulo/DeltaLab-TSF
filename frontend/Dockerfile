FROM maven:3.9-eclipse-temurin-21
WORKDIR /app
COPY . .
RUN mvn clean package
CMD ["java", "-jar", "target/frontend.jar"]