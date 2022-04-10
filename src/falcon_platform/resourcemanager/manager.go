// implement the abstract class of ResourceManager with interface + struct
package resourcemanager

import (
	"context"
	"falcon_platform/logger"
	"sync"
)

// interface for manager class
type ManagerInterface interface {

	// Monitor resource during resource runtime
	ResourceMonitor(
		resource interface{}, // resource need to be monitored
		resourceClear context.Context, // control if clear resource or not
		resourceRelease context.Context) // control if release resource or not

	// Update deployment related config if necessary
	UpdateConfig(config interface{})

	// Create resources
	CreateResource(
		cmd interface{}, // command line to be executed
		resourceClear context.Context, // control if clear resource or not
		resourceRelease context.Context, // control if release resource or not
		mux *sync.Mutex, // lock
		resourceNum *int, // store number of sub-processes
		TaskStatus *string, // store task status
		runTimeErrorLog *string, // store task error logs
	)

	// delete resources
	DeleteResource(resource interface{})
}

// subprocess/ will be named as resources
type ResourceManager struct {
	// public variables
	// lock
	Mux sync.Mutex
	// task status, record status of tasks created by "ResourceManager.CreateResources"
	TaskStatus string
	// record executing error logs of task created by "ResourceManager.CreateResources",
	// if no error, it;s ""
	RunTimeErrorLog string

	// private variables
	// total number of of resources(process or container), used to check if there are running resources
	resourceNum int
	// clear resource or not
	isClearCtx    context.Context
	clearResource context.CancelFunc
	// release resource or not,
	isReleaseCtx    context.Context
	releaseResource context.CancelFunc
}

func InitResourceManager() *ResourceManager {
	rm := new(ResourceManager)
	// default resourceNum is 0
	rm.resourceNum = 0
	// a global context to control clear or not
	rm.isClearCtx, rm.clearResource = context.WithCancel(context.Background())
	// a global context to control if monitor resource anymore
	rm.isReleaseCtx, rm.releaseResource = context.WithCancel(context.Background())
	return rm
}

/**
 * @Description create resources, can be created by many ways. depend on mngr
 * @Date 下午6:14 20/08/21
 * @Param mngr: can be k8smanager, subprocessmngr or dockermanager
 * @Param cmd: the cmd need to be executed, it's a string or cmd
 * @return
 **/
func (rm *ResourceManager) CreateResources(mngr ManagerInterface, cmd interface{}) {

	mngr.CreateResource(
		cmd,
		rm.isClearCtx, rm.isReleaseCtx,
		&rm.Mux, &rm.resourceNum,
		&rm.TaskStatus, &rm.RunTimeErrorLog)

	if len(rm.RunTimeErrorLog) != 0 {
		logger.Log.Printf("[ResourceManager]: TaskStatus is marked as = %s, CreateResources error msg is: \n============> \n%s \n<============\n", rm.TaskStatus, rm.RunTimeErrorLog)
	}
}

func (rm *ResourceManager) DeleteResources() {

	// check if there are running subprocess
	rm.Mux.Lock()

	// if there is still running job, kill the job
	if rm.resourceNum > 0 {
		rm.Mux.Unlock()
		// do the kill, cancel all resources managed by this ResourceManager
		rm.clearResource()
		logger.Log.Printf("[ResourceManager]: Wait worker kill all process/containers")

	} else {
		rm.Mux.Unlock()
	}
	logger.Log.Printf("[ResourceManager]: DeleteResources done")
}

// once release, ResourceManager can not delete it by calling rm.clearResource()
func (rm *ResourceManager) ReleaseResources() {
	logger.Log.Printf("[ResourceManager]: Release Resources, can not delete resources")
	rm.releaseResource()
}
