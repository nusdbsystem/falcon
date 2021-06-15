package taskmanager

import (
	"context"
	"falcon_platform/logger"
	"os/exec"
	"sync"
)

// subprocess/ will be named as resources
type TaskManager struct {
	Mux sync.Mutex

	// if the resources be killed
	IsKilled bool
	// number of resources
	NumProc int
	// context
	Ctx    context.Context
	Cancel context.CancelFunc

	TaskFinish chan bool
}

func InitTaskManager() *TaskManager {
	tm := new(TaskManager)
	tm.IsKilled = false
	tm.NumProc = 0
	// task manager hold a global context
	tm.Ctx, tm.Cancel = context.WithCancel(context.Background())
	tm.TaskFinish = make(chan bool)
	return tm
}

func (tm *TaskManager) CreateResources(cmd *exec.Cmd) (string, string) {
	var executeStr string
	var runTimeErrorLog string

	executeStr, runTimeErrorLog = CreateProc(cmd, tm.Ctx, &tm.Mux, &tm.NumProc)
	logger.Log.Printf("[TaskManager]: Record CreateResources results as: <'%s'> \n", executeStr)

	return executeStr, runTimeErrorLog
}

func (tm *TaskManager) DeleteResources() {

	// check if there are running subprocess
	tm.Mux.Lock()

	// if there is still running job, kill the job
	if tm.NumProc > 0 {
		tm.IsKilled = true
		tm.Mux.Unlock()

		// do the kill,  cancel all goroutines
		tm.Cancel()

		logger.Log.Printf("[TaskManager]: Wait worker kill all process/containers")
		// wait until worker finish kill logic successfully
		<-tm.TaskFinish

	} else {
		tm.Mux.Unlock()
	}
	logger.Log.Printf("[TaskManager]: DeleteResources done")
}
