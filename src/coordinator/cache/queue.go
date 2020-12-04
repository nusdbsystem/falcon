package cache

import (
	"sync"
)


type Queue struct {
	sync.Mutex

	queue []*QItem
}

func (q *Queue) Push(item *QItem) {

	q.Lock()

	q.queue = append(q.queue, item)

	q.Unlock()

}

func (q *Queue) Pop() (*QItem, bool) {
	q.Lock()
	defer q.Unlock()

	if q.isNotEmpty() {

		item := q.queue[0]

		q.queue = q.queue[1:]

		return item, true

	} else {

		return &QItem{}, false

	}
}

func (q *Queue) Clear() {
	q.Lock()
	q.queue = []*QItem{}
	q.Unlock()
}

func (q *Queue) isNotEmpty() bool {
	if len(q.queue) > 0 {
		return true
	} else {
		return false
	}
}