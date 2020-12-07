
FROM amd64/golang:1.14

#WORKDIR /go/src/app

RUN ln -snf /usr/share/zoneinfo/$TIME_ZONE /etc/localtime && echo $TIME_ZONE > /etc/timezone

COPY bin/coordinator_server ./coordinator_server
ADD deploy ./deploy
ADD scripts ./scripts
RUN chmod -R 777 ./deploy && chmod -R 777 ./scripts

CMD ["./coordinator_server"]
