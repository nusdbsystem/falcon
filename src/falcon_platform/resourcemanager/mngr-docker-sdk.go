package resourcemanager

import (
	"context"
	"falcon_platform/common"
	"falcon_platform/logger"
	"fmt"
	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/mount"
	"github.com/docker/docker/api/types/network"
	"github.com/docker/docker/client"
	"github.com/docker/docker/pkg/stdcopy"
	natting "github.com/docker/go-connections/nat"
	specs "github.com/opencontainers/image-spec/specs-go/v1"
	"os"
	"sync"
	"time"
)

// this is docker client, which is able to create or kill docker containers.
// this is isolate file, keep it, as it could be used later

type DockerContainerConfig struct {
	ContainerConfig  *container.Config
	HostConfig       *container.HostConfig
	NetworkingConfig *network.NetworkingConfig
	Platform         *specs.Platform
	Name             string
}

/**
 * @Description
 * @Date 下午2:34 21/08/21
 * @Param
		ports: key=ports inside docker, value=ip and port of host
		envs: a list of envs eg: []string{"a=a", "b=b"}
		cmd: cmd to run
		workDIr: workDir inside docker
		mountDir[0]: path in host, mountDir[1]: path in docker
 * @return
 **/
func InitDockerContainerConfig(
	ports map[natting.Port][]natting.PortBinding,
	envs []string,
	cmd []string,
	workDIr string,
	ImageName string,
	mountDir [2]string,
	Name string,
) *DockerContainerConfig {

	// 1 .port mapping
	exposedPorts := make(map[natting.Port]struct{})
	hostPorts := make(map[natting.Port][]natting.PortBinding)
	for PortIns, port := range ports {
		exposedPorts[PortIns] = struct{}{}
		hostPorts[PortIns] = port
	}

	// 3. init config instance
	config := new(DockerContainerConfig)
	config.ContainerConfig = &container.Config{
		Image:        ImageName,    //  image name
		ExposedPorts: exposedPorts, // port mapping
		Env:          envs,         // environment variables inside docker
		WorkingDir:   workDIr,      // working dir inside docker
		Cmd:          cmd,          // cmd to run
	}
	// 4 .host config
	config.HostConfig = &container.HostConfig{
		PortBindings: hostPorts, // port mapping
		RestartPolicy: container.RestartPolicy{
			Name: "no",
		},
		LogConfig: container.LogConfig{
			Type:   "json-file",
			Config: map[string]string{},
		},

		Mounts: []mount.Mount{
			{
				Type:   mount.TypeBind,
				Source: mountDir[0],
				Target: mountDir[1],
			},
		},
	}

	config.NetworkingConfig = nil
	config.Platform = nil
	config.Name = Name
	return config
}

type DockerSdkMngr struct {
	containerConfig DockerContainerConfig
}

/**
 * @Description  init InitDockerSdkMngr,
 * @Date 下午9:28 20/08/21
 * @Param
 * @return
 **/
func InitDockerSdkMngr() *DockerSdkMngr {

	dockerMngr := new(DockerSdkMngr)
	dockerMngr.containerConfig.ContainerConfig = nil
	dockerMngr.containerConfig.HostConfig = nil
	dockerMngr.containerConfig.NetworkingConfig = nil
	dockerMngr.containerConfig.Platform = nil
	dockerMngr.containerConfig.Name = ""

	return dockerMngr
}

// the ctx is shared by all threads created by worker,
// once cancel, all threads stopped
func (mngr *DockerSdkMngr) ResourceMonitor(
	resource interface{},
	isClearCtx context.Context,
	isReleaseCtx context.Context) {

	containerId := resource.(string)
loop:
	for {
		select {
		case <-isReleaseCtx.Done():
			logger.Log.Println("[DockerSdkMngr]: resources released")
			break
		case <-isClearCtx.Done():
			mngr.DeleteResource(containerId)
			break loop
		default:
			time.Sleep(time.Second * 1)
		}
	}
}

/**
 * @Description update config of specif container
 * @Date 下午9:28 20/08/21
 * @Param
 * @return
 **/
func (mngr *DockerSdkMngr) UpdateConfig(config interface{}) {
	mngr.containerConfig = *config.(*DockerContainerConfig)
}

/**
 * @Description create docker container by imageName, must call update to UpdateConfig to update the container config
				before calling this method
 * @Date 下午8:35 20/08/21
 * @Param incmd: docker image name,
 * @return
 **/
func (mngr *DockerSdkMngr) CreateResource(
	incmd interface{}, // command line to be executed
	isClearCtx context.Context, // control if clear resource or not
	isReleaseCtx context.Context, // control if release resource or not
	mux *sync.Mutex, // lock
	resourceNum *int, // store number of sub-processes
	TaskStatus *string, // store task status
	runTimeErrorLog *string, // store task error logs
) {

	imageName := incmd.(string)

	defer func() {
		logger.Log.Println("[DockerSdkMngr]: lock")
		mux.Lock()
		*resourceNum -= 1
		mux.Unlock()
		logger.Log.Println("[DockerSdkMngr]: Unlock ")
	}()

	// 1. create client instance
	ctx := context.Background()
	cli, err := client.NewClientWithOpts(client.FromEnv, client.WithAPIVersionNegotiation())
	if err != nil {
		mux.Lock()
		*runTimeErrorLog = err.Error()
		*TaskStatus = common.TaskFailed
		mux.Unlock()
		return
	}
	// 2. create containers
	resp, err := cli.ContainerCreate(
		ctx,
		mngr.containerConfig.ContainerConfig,
		mngr.containerConfig.HostConfig,
		mngr.containerConfig.NetworkingConfig,
		mngr.containerConfig.Platform,
		mngr.containerConfig.Name)

	if err != nil {
		mux.Lock()
		*runTimeErrorLog = err.Error()
		mux.Unlock()
		return
	}

	// 3. start containers
	if err := cli.ContainerStart(ctx, resp.ID, types.ContainerStartOptions{}); err != nil {
		mux.Lock()
		*runTimeErrorLog = err.Error()
		*TaskStatus = common.TaskFailed
		mux.Unlock()
		return
	}
	*TaskStatus = common.TaskSuccessful
	logger.Log.Println("[DockerSdkMngr]: started container, imageName:", imageName, "id:", resp.ID)

	// 4. start container monitor
	go func() {
		defer logger.HandleErrors()
		mngr.ResourceMonitor(resp.ID, isClearCtx, isReleaseCtx)
	}()

	mux.Lock()
	*resourceNum += 1
	mux.Unlock()

	// 5. get the container log
	out, err := cli.ContainerLogs(ctx, resp.ID, types.ContainerLogsOptions{ShowStdout: true})
	if err != nil {
		mux.Lock()
		*runTimeErrorLog = err.Error()
		*TaskStatus = common.TaskFailed
		mux.Unlock()
		return
	}
	// 6. print container log
	var outStream OutStream
	_, _ = stdcopy.StdCopy(outStream, os.Stderr, out)

	return
}

/**
 * @Description  delete a container
 * @Date 下午2:50 21/08/21
 * @Param resource: docker container id
 * @return
 **/
func (mngr *DockerSdkMngr) DeleteResource(resource interface{}) {

	containerId := resource.(string)
	// stop container by id
	ctx := context.Background()
	var err error
	cli, err := client.NewClientWithOpts(client.FromEnv, client.WithAPIVersionNegotiation())
	if err != nil {
		logger.Log.Println("[SubProcessManager]: Manually Killed containerId=", containerId, " error ", err)
	} else {
		logger.Log.Println("[SubProcessManager]: Manually Killed containerId=", containerId)
	}

	err = cli.ContainerStop(ctx, containerId, nil)

	if err != nil {
		logger.Log.Println("[SubProcessManager]: Manually Killed containerId=", containerId, " error ", err)
	} else {
		logger.Log.Println("[SubProcessManager]: Manually Killed containerId=", containerId)
	}
	logger.Log.Println("[DockerSdkMngr]: stopped container ")
}

/**
 * @Description  delete all running container
 * @Date 下午2:50 21/08/21
 * @Param resource:
 * @return
 **/
func (mngr *DockerSdkMngr) DeleteAll() {
	// stop all containers
	ctx := context.Background()
	cli, err := client.NewClientWithOpts(client.FromEnv, client.WithAPIVersionNegotiation())
	if err != nil {
		panic(err)
	}

	containers, err := cli.ContainerList(ctx, types.ContainerListOptions{})
	if err != nil {
		panic(err)
	}

	for _, con := range containers {
		fmt.Print("[DockerSdkMngr]: Stopping container ", con.ID[:10], "... ")
		if err := cli.ContainerStop(ctx, con.ID, nil); err != nil {
			panic(err)
		}
		fmt.Println("[DockerSdkMngr]: Success")
	}
}
