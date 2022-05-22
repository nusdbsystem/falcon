package tasks

import (
	"falcon_platform/common"
	"sync"
)

var allTasksOnce sync.Once
var allTasks map[common.FalconTask]Task

func GetAllTasks() map[common.FalconTask]Task {
	allTasksOnce.Do(func() {
		allTasks = make(map[common.FalconTask]Task)
	})
	return allTasks
}
