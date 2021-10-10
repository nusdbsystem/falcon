
FROM amd64/golang:1.14

#WORKDIR /go/src/app

RUN ln -snf /usr/share/zoneinfo/$TIME_ZONE /etc/localtime && echo $TIME_ZONE > /etc/timezone

RUN mkdir ./falcon_platform
COPY ./falcon_platform ./falcon_platform
#ADD deploy ./deploy
#ADD scripts ./scripts
#RUN chmod -R 777 ./deploy && chmod -R 777 ./scripts
#COPY config_partyserver.properties ./config_partyserver.properties
WORKDIR ./falcon_platform
CMD ["bash", "make_platform.sh", "Linux"]
