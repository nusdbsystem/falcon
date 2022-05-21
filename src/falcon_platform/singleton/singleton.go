package singleton

import (
	"falcon_platform/cache"
	"os"
	"sync"
	"log"

)






var logOnce sync.Once
var Log *log.Logger
var LogFile *os.File


var jobQueueOnce sync.Once
var JobQueue *cache.JobQueue

func GetJobQueue() *cache.JobQueue {
	jobQueueOnce.Do(func() {
		JobQueue = &cache.JobQueue{}
	})
	return JobQueue
}

var jobQueueOnce sync.Once
var JobQueue *cache.JobQueue

func GetJobQueue() *cache.JobQueue {
	jobQueueOnce.Do(func() {
		JobQueue = &cache.JobQueue{}
	})
	return JobQueue
}
