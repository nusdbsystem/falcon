package taskmanager

import (
	"context"
	"falcon_platform/distributed/utils"
	"falcon_platform/logger"
	"fmt"
	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/client"
	"github.com/docker/docker/pkg/stdcopy"
	"os"
	"sync"
	"time"
)

// this is docker client, which is able to create or kill docker containers.
// this is isolate file, keep it, as it could be used later

func ContainerMonitor(id string, Ctx context.Context) {
	// the ctx is shared by all threads created by worker,
	// once cancel, all threads stopped
loop:
	for {
		select {
		case <-Ctx.Done():
			err := stopContainer(id)
			if err != nil {
				logger.Log.Println("[SubProcessManager]: Manually Killed containerId=", id, " error ", err)
			} else {
				logger.Log.Println("[SubProcessManager]: Manually Killed containerId=", id)
			}
			break loop
		default:
			time.Sleep(time.Second * 1)
		}
	}
}

func CreateContainer(
	imageName string,
	Ctx context.Context,
	mux *sync.Mutex,
	NumProc *int,

) (executeStr string, runTimeErrorLog string) {

	defer func() {
		logger.Log.Println("[DockerManager]: lock")
		mux.Lock()
		*NumProc -= 1
		mux.Unlock()
		logger.Log.Println("[DockerManager]: Unlock ")
	}()

	// start containers
	ctx := context.Background()
	cli, err := client.NewClientWithOpts(client.FromEnv, client.WithAPIVersionNegotiation())
	if err != nil {
		executeStr = err.Error()
		return
	}
	resp, err := cli.ContainerCreate(ctx, &container.Config{
		Image: imageName,
	}, nil, nil, nil, "")

	if err != nil {
		executeStr = err.Error()
		return
	}

	if err := cli.ContainerStart(ctx, resp.ID, types.ContainerStartOptions{}); err != nil {
		executeStr = err.Error()
		return
	}
	logger.Log.Println("[DockerManager]: started container, name:", imageName, "id:", resp.ID)

	go ContainerMonitor(resp.ID, Ctx)
	mux.Lock()
	*NumProc += 1
	mux.Unlock()

	//statusCh, errCh := cli.ContainerWait(ctx, resp.ID, container.WaitConditionNotRunning)
	//select {
	//case err := <-errCh:
	//	if err != nil {
	//		panic(err)
	//	}
	//case <-statusCh:
	//}

	// get the container log
	out, err := cli.ContainerLogs(ctx, resp.ID, types.ContainerLogsOptions{ShowStdout: true})
	if err != nil {
		executeStr = err.Error()
		return
	}
	// print container log
	var outStream utils.OutStream
	_, _ = stdcopy.StdCopy(outStream, os.Stderr, out)

	return
}

func stopContainer(containerId string) error {
	// stop container by id
	ctx := context.Background()
	cli, err := client.NewClientWithOpts(client.FromEnv, client.WithAPIVersionNegotiation())
	if err != nil {
		return err
	}

	if err := cli.ContainerStop(ctx, containerId, nil); err != nil {
		return err
	}
	logger.Log.Println("[DockerManager]: stopped container ")
	return nil
}

func StopAll() {
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
		fmt.Print("[DockerManager]: Stopping container ", con.ID[:10], "... ")
		if err := cli.ContainerStop(ctx, con.ID, nil); err != nil {
			panic(err)
		}
		fmt.Println("[DockerManager]: Success")
	}
}
