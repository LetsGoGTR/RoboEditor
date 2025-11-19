FROM drogonframework/drogon:latest
USER root

# 1) 미러를 한국(또는 카카오) 쪽으로 바꿔주고
RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://kr.archive.ubuntu.com/ubuntu/|g' /etc/apt/sources.list

# 2) update + 설치를 한 RUN에 묶기
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        cmake build-essential git \
        libssl-dev libjsoncpp-dev uuid-dev zlib1g-dev \
        libboost-all-dev libarchive-dev libyaml-cpp-dev libssh2-1-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY Drogon/ /app

RUN mkdir build && cd build && cmake .. && make
WORKDIR /app/build

CMD ["./Drogon"]
