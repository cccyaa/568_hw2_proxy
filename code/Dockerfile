FROM gcc
RUN mkdir /code
RUN mkdir /var/log/erss
WORKDIR /code
RUN apt-get update && apt-get install -y g++
COPY . /code