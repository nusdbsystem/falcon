package resourcemanager

import (
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/logger"
	"fmt"
	"io/ioutil"
	"math/rand"
	"net"
	"os/exec"
	"strings"
)

// try with the port, if it's available, use it, otherwise retry until find an available port
func GetFreePort(portNum int) []common.PortType {
	Ntime := 5
	var portRes []common.PortType
	var err error

	for i := 0; i < Ntime; i++ {
		// get free port from coordinator
		portArray := client.GetFreePort(common.CoordAddr, portNum)

		for _, port := range portArray {
			err = CheckPort(port)
			if err != nil {
				break
			}
		}

		if err != nil {
			continue
		}
		// if success, break
		portRes = portArray
		break
	}

	// if l is still null, panic out
	if err != nil {
		panic(fmt.Sprintf("[ResourceManager]: Get free port error:%s", err))
	}

	return portRes
}

func CheckPort(port common.PortType) error {
	// try the port,
	addr, err := net.ResolveTCPAddr("tcp", fmt.Sprintf("localhost:%d", port))
	if err != nil {
		logger.Log.Printf("[ResourceManager]: Get free port error: %s\n", err)
		return err
	}
	l, err := net.ListenTCP("tcp", addr)
	if err != nil {
		logger.Log.Printf("[ResourceManager]: Get free port error: %s\n", err)
		return err
	}

	// if l is still null, panic out
	defer func() {
		_ = l.Close()
	}()

	return nil
}

func GetMpcExecutorPort(workerID int, stageName string, classID int) common.PortType {
	stagePrefix := 1
	if stageName == string(common.PreProcStage) {
		stagePrefix = 2
	}

	if stageName == string(common.ModelTrainStage) {
		stagePrefix = 3
	}

	if stageName == string(common.LimeInstanceSampleStage) {
		stagePrefix = 4
	}

	if stageName == string(common.LimePredStage) {
		stagePrefix = 5
	}

	if stageName == string(common.LimeWeightStage) {
		stagePrefix = 6
	}

	stageName = strings.Split(stageName, "-")[0]

	if stageName == string(common.LimeFeatureSelectionStage) {
		stagePrefix = 7
	}

	if stageName == string(common.LimeVFLModelTrainStage) {
		stagePrefix = 8
	}

	prefix := stagePrefix*1000 + workerID*100 + classID*10

	return common.PortType(common.MpcExecutorBasePort + prefix)
}

// get many ports
func GetFreePorts(count int) ([]int, error) {
	var ports []int
	for i := 0; i < count; i++ {
		addr, err := net.ResolveTCPAddr("tcp", "localhost:0")
		if err != nil {
			return nil, err
		}

		l, err := net.ListenTCP("tcp", addr)
		if err != nil {
			return nil, err
		}
		ports = append(ports, l.Addr().(*net.TCPAddr).Port)
		logger.Log.Println(l.Close())
	}
	return ports, nil
}

// get one port
func GetFreePort4K8s() (int, error) {
	min := 30000
	max := 32767
	p := rand.Intn(max-min) + min

	var addr *net.TCPAddr
	var l *net.TCPListener
	var err error

	for {
		addr, err = net.ResolveTCPAddr("tcp", "localhost:"+fmt.Sprintf("%d", p))
		if err != nil {
			return 0, err
		}

		l, err = net.ListenTCP("tcp", addr)
		if err != nil {
			p++
		} else {
			logger.Log.Println(l.Close())
			return p, nil
		}
	}
}

type OutStream struct{}

func (out OutStream) Write(p []byte) (int, error) {
	fmt.Printf("[SubProcessManager]: subprocess' output log : \n============> \n%s \n<============\n", string(p))
	return len(p), nil
}

func ExecuteBash(command string) error {
	// return cmd object
	logger.Log.Println("[ExecuteBash]: execute bash ::", command)

	cmd := exec.Command("bash", "-c", command)

	stderr, _ := cmd.StderrPipe()
	stdout, _ := cmd.StdoutPipe()

	if err := cmd.Start(); err != nil {
		logger.Log.Println("[ExecuteBash]: executeBash: Start error ", err)
		return err
	}
	errLog, _ := ioutil.ReadAll(stderr)
	outLog, _ := ioutil.ReadAll(stdout)

	logger.Log.Println("[ExecuteBash]: executeBash: error log is ", string(errLog))
	logger.Log.Println("[ExecuteBash]: executeBash: out put is ", string(outLog))
	outErr := cmd.Wait()
	return outErr

}

func ExecuteCmd(cmd *exec.Cmd) error {
	// cmd

	//logger.Log.Printf("[ExecuteCmd]: Execute cmd \"%s\"", cmd.String())
	printCmd(cmd.String())

	stderr, _ := cmd.StderrPipe()
	stdout, _ := cmd.StdoutPipe()

	if err := cmd.Start(); err != nil {
		logger.Log.Println("[ExecuteCmd]: Error: ", err)
		return err
	}
	errLog, _ := ioutil.ReadAll(stderr)
	outLog, _ := ioutil.ReadAll(stdout)

	logger.Log.Println("[ExecuteCmd]: ErrorLog is ", string(errLog))
	logger.Log.Println("[ExecuteCmd]: OutPut is ", string(outLog))
	outErr := cmd.Wait()

	return outErr
}

func printCmd(cmd string) {
	res := strings.Split(cmd, "--")
	show := strings.Join(res, "\\\n--")
	logger.Log.Printf("[ExecuteCmd]: %s\n", show)
}
