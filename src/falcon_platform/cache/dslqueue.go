package cache

import (
	"sync"
)

type DslQueue struct {
	sync.Mutex

	dslqueue []*DslObj
}

func (q *DslQueue) Push(item *DslObj) {

	q.Lock()

	q.dslqueue = append(q.dslqueue, item)

	q.Unlock()

}

func (q *DslQueue) Pop() (*DslObj, bool) {
	q.Lock()
	defer q.Unlock()

	if q.isNotEmpty() {

		item := q.dslqueue[0]

		q.dslqueue = q.dslqueue[1:]

		return item, true

	} else {

		return &DslObj{}, false

	}
}

func (q *DslQueue) Clear() {
	q.Lock()
	q.dslqueue = []*DslObj{}
	q.Unlock()
}

func (q *DslQueue) isNotEmpty() bool {
	if len(q.dslqueue) > 0 {
		return true
	} else {
		return false
	}
}
