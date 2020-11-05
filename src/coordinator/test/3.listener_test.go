package test

import (
	"coordinator/config"
	"coordinator/listener"
	"testing"
)

func TestWorker(t *testing.T) {

	localIp := "172.25.122.185"
	masterAddr := localIp + ":" + config.MasterPort

	listener.SetupListener(localIp, config.ListenerPort, masterAddr)
}
