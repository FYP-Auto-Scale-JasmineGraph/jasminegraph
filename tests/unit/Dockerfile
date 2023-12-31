FROM ishad1999/jasminegraph

RUN python3 -m pip install --no-cache-dir gcovr

ENV TERM=xterm-256color
ENV HOME="/home/ubuntu"
ENV JASMINEGRAPH_HOME="${HOME}/software/jasminegraph"

WORKDIR "${JASMINEGRAPH_HOME}"

COPY ./cmake_modules ./cmake_modules
COPY ./build.sh ./build.sh
COPY ./CMakeLists.txt ./CMakeLists.txt
COPY ./src_python ./src_python
COPY ./src ./src
COPY ./main.h ./main.h
COPY ./main.cpp ./main.cpp
COPY ./conf ./conf
RUN mkdir build
RUN mkdir tests
COPY ./tests/unit ./tests/unit
COPY ./k8s ./k8s

WORKDIR "${JASMINEGRAPH_HOME}/build"

RUN cmake -DCMAKE_BUILD_TYPE=DEBUG ..
RUN make -j4

CMD ["bash", "-c", "LD_LIBRARY_PATH=/usr/local/lib make -j2 coverage && cp coverage.xml ../coverage/"]