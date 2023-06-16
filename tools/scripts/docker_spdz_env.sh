apt-get update
apt-get -y install python3-pip
pip3 install numpy
pip3 install scipy
pip3 install -U scikit-learn
apt-get install vim

bash Scripts/setup-online.sh 2 128 128 && bash Scripts/setup-clients.sh 2 && bash Scripts/setup-ssl.sh 2 128 128 && c_rehash Player-Data/
bash Scripts/setup-online.sh 3 128 128 && bash Scripts/setup-clients.sh 3 && bash Scripts/setup-ssl.sh 3 128 128 && c_rehash Player-Data/
bash Scripts/setup-online.sh 4 128 128 && bash Scripts/setup-clients.sh 4 && bash Scripts/setup-ssl.sh 4 128 128 && c_rehash Player-Data/
bash Scripts/setup-online.sh 5 128 128 && bash Scripts/setup-clients.sh 5 && bash Scripts/setup-ssl.sh 5 128 128 && c_rehash Player-Data/
bash Scripts/setup-online.sh 6 128 128 && bash Scripts/setup-clients.sh 6 && bash Scripts/setup-ssl.sh 6 128 128 && c_rehash Player-Data/

docker commit --change='ENTRYPOINT ["bash", "deployment/docker_cmd.sh"]' 49e7f7054059 lemonwyc/falcon:3party

# if build environment for spdz baseline, add the following to the dockerfile

: '
# temp for mpdz baseline. update mpc data and code
# Client-side handshake with P0 failed. Make sure we have the necessary certificate (Player-Data/P0.pem in the default configuration), and run `c_rehash <directory>` on its location.
# The certificates should be the same on every host. Also make sure that it's still valid. Certificates generated with `Scripts/setup-ssl.sh` expire after a month.
WORKDIR /opt/falcon/third_party/MP-SPDZ
RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y --no-install-recommends python3-pip && \
    pip3 install numpy && pip3 install -U scikit-learn && \
    git fetch origin && \
    git checkout -b baseline origin/baseline && \
    Scripts/setup-online.sh 2 128 128 && \
    Scripts/setup-clients.sh 2 && \
    Scripts/setup-ssl.sh 2 128 128 && \
    c_rehash Player-Data/ && \
    git pull && \
    git log --oneline -2 && \
    bash compile_secureml.sh
'