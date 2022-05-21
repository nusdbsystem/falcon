package cache

import (
	"falcon_platform/common"
	"sync"
)

var jobQueueOnce sync.Once
var jobQueueIns *JobQueue

func GetJobQueue() *JobQueue {
	jobQueueOnce.Do(func() {
		jobQueueIns = &JobQueue{}
	})
	return jobQueueIns
}

type JobQueue struct {
	sync.Mutex

	jobQueue []*common.TrainJob
}

func (q *JobQueue) Push(item *common.TrainJob) {

	q.Lock()

	q.jobQueue = append(q.jobQueue, item)

	q.Unlock()

}

func (q *JobQueue) Pop() (*common.TrainJob, bool) {
	q.Lock()
	defer q.Unlock()

	if q.isNotEmpty() {

		item := q.jobQueue[0]

		q.jobQueue = q.jobQueue[1:]

		return item, true

	} else {

		return &common.TrainJob{}, false

	}
}

func (q *JobQueue) Clear() {
	q.Lock()
	q.jobQueue = []*common.TrainJob{}
	q.Unlock()
}

func (q *JobQueue) isNotEmpty() bool {
	if len(q.jobQueue) > 0 {
		return true
	} else {
		return false
	}
}
