package test

import (
	"coordinator/common"
	"coordinator/listener"
	"testing"
)

func TestWorker(t *testing.T) {

	localIp := "172.25.122.185"
	masterAddr := localIp + ":" + common.MasterPort

	listener.SetupListener(localIp, common.ListenerPort, masterAddr)
}
