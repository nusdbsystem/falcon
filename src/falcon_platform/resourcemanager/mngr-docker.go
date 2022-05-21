/**
 * @Description:
 * @File:  mngr-docker
 * @Version: 1.0.0
 * @Date: 22/08/21 下午7:50
 */
package resourcemanager

import (
	"context"
	"os/exec"
	"sync"
)

type DockerManager struct{}

func InitDockerManager() *DockerManager {
	return new(DockerManager)
}

func (mngr *DockerManager) ResourceMonitor(
	resource interface{},
	isClearCtx context.Context,
	isReleaseCtx context.Context) {
}

func (mngr *DockerManager) UpdateConfig(config interface{}) {}

/**
 * @Description
 * @Date 下午7:53 22/08/21
 * @Param
 * @return
 **/
func (mngr *DockerManager) CreateResource(
	mCmd interface{}, // command line to be executed
	isClearCtx context.Context, // control if clear resource or not
	isReleaseCtx context.Context, // control if release resource or not
	mux *sync.Mutex, // lock
	resourceNum *int, // store number of sub-processes
	TaskStatus *string, // store tasks status
	runTimeErrorLog *string, // store tasks error logs
) {

	cmd := mCmd.(*exec.Cmd)
	e := ExecuteCmd(cmd)
	if e != nil {
		panic("[DockerManager]: Error: " + e.Error())
	}

}

func (mngr *DockerManager) DeleteResource(resource interface{}) {}
