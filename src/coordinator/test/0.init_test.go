package test

import (
	"coordinator/logger"
	"fmt"
)

func init(){
	fmt.Println("Init logger")
	logger.Do, logger.F = logger.GetLogger("./TestSubProc")

}
