# Setups

1. run redis locally

   ```bash
   docker pull redis
   docker run -d --name redis --net=host redis
   ```

2. inside PaddleFL docker, install redis cli inside the paddle docker

   ```bash
   wget http://download.redis.io/redis-stable.tar.gz 
   tar xvzf redis-stable.tar.gz
   cd redis-stable
   make
   sudo cp src/redis-cli /usr/local/bin/
   /usr/local/bin/redis-cli
   ```

3. run padddleFL docker

   ```bash
   docker pull paddlepaddle/paddlefl:1.1.2
   # go the the local paddle code dir, edit and update
   docker run --name paddle_exp --net=host -it -v $PWD:/paddle paddlepaddle/paddlefl:1.1.2 /bin/bash
   ```

4. inside PaddleFL docker, config the env

   ```bash
   export PYTHON=/usr/bin/python3
   export PATH_TO_REDIS_BIN=/paddle/redis-stable/src
   export LOCALHOST=127.0.0.7
   export REDIS_PORT=6379
   ```

5. inside PaddleFL docker, run script

   ```bash
   cd /paddle/PaddleFL/python/paddle_fl/mpc/examples
   bash run_standalone.sh linear_reg_with_uci/uci_demo.py
   ```

   

