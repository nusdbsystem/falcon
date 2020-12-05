FROM golang:1.14

RUN ln -snf /usr/share/zoneinfo/$TIME_ZONE /etc/localtime && echo $TIME_ZONE > /etc/timezone

COPY bin/coordinator_server ./coordinator_server

CMD ["./coordinator_server"]
