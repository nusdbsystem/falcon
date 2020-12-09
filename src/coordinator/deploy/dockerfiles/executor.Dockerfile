
FROM amd64/golang:1.14

#WORKDIR /go/src/app

RUN ln -snf /usr/share/zoneinfo/$TIME_ZONE /etc/localtime && echo $TIME_ZONE > /etc/timezone


RUN apt-get update && apt-get -y upgrade
# Install conda with pip and python 3.6
RUN apt-get -y install curl bzip2 \
  && curl -sSL https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh -o /tmp/miniconda.sh \
  && bash /tmp/miniconda.sh -bfp /usr/local \
  && rm -rf /tmp/miniconda.sh \
  && conda create -y --name falconenv python=3.6 \
  && conda clean --all --yes

ENV PATH /usr/local/envs/falconenv/bin:$PATH
RUN pip install --upgrade pip

# remove the cahce, == "python -u"
ENV PYTHONUNBUFFERED 1

COPY bin/coordinator_server ./coordinator_server

COPY testggg.go ./testggg.go
COPY falcon_ml/preprocessing.py ./preprocessing.py

ADD deploy ./deploy
ADD scripts ./scripts
RUN chmod -R 777 ./deploy && chmod -R 777 ./scripts
CMD ["./coordinator_server"]
