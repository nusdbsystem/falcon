package test

import (
	"coordinator/logger"
	"fmt"
)

func init() {
	fmt.Println("Init logger")
	logger.Log, logger.LogFile = logger.GetLogger("./TestSubProc")

}
