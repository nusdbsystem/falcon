package controller

import (
	"falcon_platform/common"
	"falcon_platform/coordserver/entity"
	"falcon_platform/coordserver/models"
	"falcon_platform/logger"
	"strconv"
	"sync"
)

var usedPorts = make(map[int]struct{})
var currentMaxPort = 22000
var portMutex sync.Mutex

// portNum: how many port needed
func AssignPort(ctx *entity.Context, portNum string) []common.PortType {

	var res []common.PortType

	portNumInt, err := strconv.Atoi(portNum)

	if err != nil {
		panic(err)
	}

	portMutex.Lock()
	portCount := 0

	for {
		currentMaxPort = currentMaxPort + 1
		if _, ok := usedPorts[currentMaxPort]; ok {
			continue
		} else {
			usedPorts[currentMaxPort] = struct{}{}
			res = append(res, common.PortType(currentMaxPort))
			portCount += 1
		}
		if portCount == portNumInt {
			break
		}
	}

	logger.Log.Println("AssignPort: Current Port usage: ", usedPorts)
	portMutex.Unlock()

	return res
}

func AssignPortPersistent(ctx *entity.Context) uint {

	ports := ctx.JobDB.GetPorts()

	tx := ctx.JobDB.DB.Begin()

	maxPort := findMax(ports)
	if maxPort < 12000 {
		maxPort = 12000
	}

	var err error
	var u *models.PortRecord
	assignedPort := maxPort + 10
	for {
		err, u = ctx.JobDB.AddPort(tx, assignedPort)
		if err != nil {
			logger.Log.Println("AssignPort, retry..., ", err)
			assignedPort++
		} else {
			//logger.Log.Println("AssignPort: AssignSuccessful port is ", u.Port)
			break
		}
	}

	ctx.JobDB.Commit(tx, err)

	return u.Port
}

func AddPort(newPort uint, ctx *entity.Context) uint {

	tx := ctx.JobDB.DB.Begin()

	e, _ := ctx.JobDB.AddPort(tx, newPort)

	ctx.JobDB.Commit(tx, e)

	return newPort

}

func findMax(l []uint) uint {
	var res uint

	for j := 0; j < len(l)-1; j++ {

		if l[j] > l[j+1] {
			res = l[j]
		} else {
			res = l[j+1]
		}
	}
	return res
}

func GetPartyServerPort(PartyServerAddr string, ctx *entity.Context) string {

	e, u := ctx.JobDB.PartyServerGet(PartyServerAddr)
	if e != nil {
		panic(e)
	}
	return u.Port

}
