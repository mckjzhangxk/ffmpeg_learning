FROM ubuntu:bionic

WORKDIR /root
RUN apt update && apt install -y build-essential git libpcre3-dev libssl-dev zlib1g-dev
RUN git clone https://github.com/arut/nginx-rtmp-module.git && git clone https://github.com/nginx/nginx.git
RUN cd nginx && ./auto/configure --add-module=../nginx-rtmp-module && make -j && make install

COPY ./nginx.conf /usr/local/nginx/conf/nginx.conf
CMD /usr/local/nginx/sbin/nginx
# CMD [ "/bin/bash" ]
EXPOSE 1935 8080
