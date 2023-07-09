FROM ubuntu:focal
WORKDIR /home/ubuntu
RUN mkdir software
WORKDIR /home/ubuntu/software

RUN apt-get update
RUN apt-get install -y apt-transport-https
RUN apt-get update
RUN apt-get install -y curl gnupg2 ca-certificates software-properties-common nlohmann-json3-dev
RUN apt-get install -y git cmake build-essential sqlite3 libsqlite3-dev libssl-dev librdkafka-dev libboost-all-dev libtool libxerces-c-dev libflatbuffers-dev python3-pip
RUN add-apt-repository ppa:deadsnakes/ppa
RUN apt-get install -y python3.5-dev
RUN apt-get install -y libjsoncpp-dev libspdlog-dev pigz

RUN curl -fsSL https://download.docker.com/linux/debian/gpg | apt-key add -
RUN add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
RUN apt-get update
RUN apt-get install -y docker-ce-cli

RUN git clone https://github.com/chinthakarukshan/metis.git
RUN git clone https://github.com/mfontanini/cppkafka.git

WORKDIR /home/ubuntu/software/metis
RUN tar -xzf metis-5.1.0.tar.gz
WORKDIR /home/ubuntu/software/metis/metis-5.1.0
RUN sed -i '/#define IDXTYPEWIDTH 32/c\#define IDXTYPEWIDTH 64' include/metis.h
RUN make config shared=1 cc=gcc
RUN make -j4 install

RUN mkdir /home/ubuntu/software/cppkafka/build
WORKDIR /home/ubuntu/software/cppkafka/build
RUN cmake ..
RUN make -j4
RUN make install

ENV HOME="/home/ubuntu"
ENV JASMINEGRAPH_HOME="/home/ubuntu/software/jasminegraph"
RUN mkdir /home/ubuntu/software/jasminegraph
WORKDIR /home/ubuntu/software/jasminegraph
RUN pip install tensorflow==2.5.3
RUN pip install -U scikit-learn
COPY . .
RUN pip install -r ./GraphSAGE/requirements

RUN sh build.sh
ENTRYPOINT ["/home/ubuntu/software/jasminegraph/run-docker.sh"]
CMD ["bash"]
