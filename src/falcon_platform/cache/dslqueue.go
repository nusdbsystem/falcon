package cache

import (
	"falcon_platform/common"
	"sync"
)

type DslQueue struct {
	sync.Mutex

	dslqueue []*common.TrainJob
}

func (q *DslQueue) Push(item *common.TrainJob) {

	q.Lock()

	q.dslqueue = append(q.dslqueue, item)

	q.Unlock()

}

func (q *DslQueue) Pop() (*common.TrainJob, bool) {
	q.Lock()
	defer q.Unlock()

	if q.isNotEmpty() {

		item := q.dslqueue[0]

		q.dslqueue = q.dslqueue[1:]

		return item, true

	} else {

		return &common.TrainJob{}, false

	}
}

func (q *DslQueue) Clear() {
	q.Lock()
	q.dslqueue = []*common.TrainJob{}
	q.Unlock()
}

func (q *DslQueue) isNotEmpty() bool {
	if len(q.dslqueue) > 0 {
		return true
	} else {
		return false
	}
}
