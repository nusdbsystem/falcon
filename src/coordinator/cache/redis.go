package cache

import (
	"context"
	"coordinator/common"
	"github.com/go-redis/redis/v8"
	"time"
)

/**
 * @Author
 * @Description
 * @Date 4:31 下午 4/12/20
 * @Param  this is the shared memory between different process, in production.
 * @return
 **/


type RedisSession struct {
	
	redisCli	*redis.Client
		 ctx    context.Context
}

func InitRedisClient() *RedisSession {
	rdb := redis.NewClient(&redis.Options{
		Addr:     common.RedisHost + ":" + common.RedisPort,
		Password: common.RedisPwd, // no password set
		DB:       0,  // use default DB
	})
	
	rs := RedisSession{
		redisCli: rdb,
		ctx: context.Background(),
	}
	
	return &rs
}


func (rs *RedisSession) Set(key, value string){
	err := rs.redisCli.Set(rs.ctx, key, value, 10*time.Minute).Err()
	if err != nil {
		panic(err)
	}
}


func (rs *RedisSession) Get(key string) string {
	val, err := rs.redisCli.Get(rs.ctx, key).Result()
	if err != nil {
		panic(err)
	}
	return val
}





