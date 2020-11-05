package entity

import (
	"coordinator/config"
	"sync"
)

type Queue struct {
	sync.Mutex

	queue []*config.QItem
}

func (q *Queue) Push(item *config.QItem) {

	q.Lock()

	q.queue = append(q.queue, item)

	q.Unlock()

}

func (q *Queue) Pop() (*config.QItem, bool) {
	q.Lock()
	defer q.Unlock()

	if q.isNotEmpty() {

		item := q.queue[0]

		q.queue = q.queue[1:]

		return item, true

	} else {

		return &config.QItem{}, false

	}
}

func (q *Queue) Clear() {
	q.Lock()
	q.queue = []*config.QItem{}
	q.Unlock()
}

func (q *Queue) isNotEmpty() bool {
	if len(q.queue) > 0 {
		return true
	} else {
		return false
	}
}
