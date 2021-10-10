package test

import (
	"context"
	"encoding/base64"
	"falcon_platform/common"
	v0 "falcon_platform/common/proto/v0"
	"falcon_platform/resourcemanager"
	"fmt"
	"github.com/docker/docker/api/types"
	"github.com/docker/docker/client"
	natting "github.com/docker/go-connections/nat"
	"github.com/golang/protobuf/proto"
	_ "github.com/opencontainers/image-spec/specs-go/v1"
	"math/rand"
	"testing"
)

func TestDockerContainer(t *testing.T) {

	rm := resourcemanager.InitResourceManager()
	dm := resourcemanager.InitDockerSdkMngr()

	port1 := "3331"
	port2 := "3332"
	port3 := "3333"
	newport, err := natting.NewPort("tcp", port1)
	newport2, err := natting.NewPort("tcp", port2)
	newport3, err := natting.NewPort("tcp", port3)
	if err != nil {
	}

	ports := make(map[natting.Port][]natting.PortBinding)
	ports[newport] = []natting.PortBinding{{HostIP: "0.0.0.0", HostPort: port1}}
	ports[newport2] = []natting.PortBinding{{HostIP: "0.0.0.0", HostPort: port2}}
	ports[newport3] = []natting.PortBinding{{HostIP: "0.0.0.0", HostPort: port3}}

	envs := []string{
		fmt.Sprintf("NewEnv=%s", "NewEnvTest"),
		fmt.Sprintf("NewEnv2=%s", "NewEnvTest2"),
		fmt.Sprintf("NewEnv3=%s", "NewEnvTest3"),
	}
	cmd := []string{"python", "readwrite.py", "-i=1", "-o=2", "-model=3"}
	workDIr := "/go"
	ImageName := "test:1"
	mountDir := [2]string{
		"/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/falcon_logs/logs",
		"/go/trainDataOutput"}
	containerName := "testContainer"

	config := resourcemanager.InitDockerContainerConfig(
		ports,
		envs,
		cmd,
		workDIr,
		ImageName,
		mountDir,
		containerName)

	dm.UpdateConfig(config)
	rm.CreateResources(dm, "test:1")
}

func TestDockerBash(t *testing.T) {

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

func TestNetworkCfg(t *testing.T) {
	a := "CgkxMjcuMC4wLjEKCTEyNy4wLjAuMQoJMTI3LjAuMC4xEgsKCfWrAfarAferARILCgmHrAGIrAGJrAESCwoJmawBmqwBm6wBGgA="

	res, _ := base64.StdEncoding.DecodeString(a)

	cfg := v0.NetworkConfig{}

	_ = proto.Unmarshal(res, &cfg)

	fmt.Println(cfg.Ips)
	fmt.Println(cfg.ExecutorExecutorPortArrays)
	fmt.Println(cfg.ExecutorMpcPortArray)
}

func TestPortRequest(t *testing.T) {

	common.CoordAddr = "127.0.0.1:30004"

	for {
		pn := rand.Intn(5)
		res := resourcemanager.GetFreePort(pn)
		fmt.Println(res)

	}
}
