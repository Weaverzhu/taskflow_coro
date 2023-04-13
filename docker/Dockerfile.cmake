FROM ubuntu:jammy

RUN sed -i 's/archive.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list \
    && sed -i '/^deb.*security.ubuntu.com/d' /etc/apt/sources.list

RUN apt update && apt -y install git cmake build-essential ca-certificates python3 wget curl sudo unzip

ADD ./scripts ./scripts
RUN ./scripts/folly.sh