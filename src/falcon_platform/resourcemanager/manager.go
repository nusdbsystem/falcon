package resourcemanager

import (
	"context"
	"falcon_platform/logger"
	"os/exec"
	"sync"
)

// subprocess/ will be named as resources
type ResourceManager struct {
	Mux sync.Mutex

	// total number of of resources(process or container)
	TotResources int
	// context
	ResourceCtx context.Context
	// kill all resources
	ResourceClear context.CancelFunc

	//task status
	TaskStatus string
	//task executing error logs, if no error, it;s ""
	RunTimeErrorLog string
}

func InitResourceManager() *ResourceManager {
	tm := new(ResourceManager)
	tm.TotResources = 0
	// task manager hold a global context
	tm.ResourceCtx, tm.ResourceClear = context.WithCancel(context.Background())
	return tm
}

func (tm *ResourceManager) CreateResources(cmd *exec.Cmd) {
	CreateProc(cmd, tm.ResourceCtx, &tm.Mux, &tm.TotResources, &tm.TaskStatus, &tm.RunTimeErrorLog)
	logger.Log.Printf("[ResourceManager]: CreateResources error msg is: <'%s'> \n", tm.RunTimeErrorLog)
}

func (tm *ResourceManager) DeleteResources() {

	// check if there are running subprocess
	tm.Mux.Lock()

	// if there is still running job, kill the job
	if tm.TotResources > 0 {
		tm.Mux.Unlock()
		// do the kill,  cancel all goroutines
		tm.ResourceClear()
		logger.Log.Printf("[ResourceManager]: Wait worker kill all process/containers")

	} else {
		tm.Mux.Unlock()
	}
	logger.Log.Printf("[ResourceManager]: DeleteResources done")
}
