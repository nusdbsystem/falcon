package test

import (
	"falcon_platform/logger"
	"fmt"
)

func init() {
	fmt.Println("Init logger")
	logger.Log, logger.LogFile = logger.GetLogger("./TestSubProc")

}
