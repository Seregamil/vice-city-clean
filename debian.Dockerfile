FROM debian:bookworm-slim as build
LABEL authors="cxxy"

USER root
WORKDIR build

RUN \
   apt update -y && \
   apt upgrade -y && \
   apt install -y \
          make \
          gcc \
          gcc-multilib \
          g++ \
          g++-multilib \
          libopenal-dev \
          libglew-dev \
          libglfw3-dev \
          libsndfile1-dev \
          libmpg123-dev \
          build-essential


# ln -s /usr/include/x86_64-linux-gnu/mpg123.h /usr/include/mpg123.h
# ln -s /usr/include/x86_64-linux-gnu/fmt123.h /usr/include/fmt123.h
# ln -s /usr/include/x86_64-linux-gnu/out123.h /usr/include/out123.h
# ln -s /usr/include/x86_64-linux-gnu/syn123.h /usr/include/syn123.h
#RUN \
#    chmod a+x premake5Linux && \
#    premake5Linux --with-librw gmake2 && \
#    cd build && \
#    make all

