package client

import (
	"falcon_platform/logger"
	"fmt"
	"testing"
)

func TestClientGetStatus(t *testing.T) {

	logger.Log, logger.LogFile = logger.GetLogger("./TestSubProc")

	status := GetJobStatus("127.0.0.1:30004", 1)
	fmt.Println(status)

}
