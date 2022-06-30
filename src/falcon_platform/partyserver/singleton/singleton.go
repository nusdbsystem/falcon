package singleton

import (
	"falcon_platform/client"
	"falcon_platform/common"
	"sync"
)

// partyNetCfg Global var,
var serverJobsOnce sync.Once
var serverJobs map[string]map[uint]struct{}

var pLock sync.Mutex

func GetServerJobInfo() map[string]map[uint]struct{} {
	serverJobsOnce.Do(func() {
		serverJobs = make(map[string]map[uint]struct{})
		for _, label := range common.PartyServerClusterLabels {
			serverJobs[label] = make(map[uint]struct{})
		}
	})
	return serverJobs
}

func addJobToServer(label string, jobID uint) {
	pLock.Lock()
	serverJobs[label][jobID] = struct{}{}
	pLock.Unlock()
}

func removeCompletedJobFromServer() {
	pLock.Lock()
	for label, jobids := range serverJobs {
		for jobid, _ := range jobids {
			status := client.GetJobStatus(common.CoordAddr, jobid)
			if status == "finished" || status == "failed" {
				delete(serverJobs[label], jobid)
			}
		}
	}
	pLock.Unlock()
}

func FindIdleServer() (bool, string) {
	removeCompletedJobFromServer()
	pLock.Lock()
	for label, jobids := range serverJobs {
		if len(jobids) == 0 {
			pLock.Unlock()
			return true, label
		}
	}
	pLock.Unlock()
	return false, ""
}

// todo: this is assume one server can only handle one worker, if not, it will panic.
func ScheduleToIdleServer(jobID int) (bool, string) {
	removeCompletedJobFromServer()
	pLock.Lock()
	for label, jobids := range serverJobs {
		if len(jobids) == 0 {
			// take the place
			serverJobs[label][uint(jobID)] = struct{}{}
			pLock.Unlock()
			return true, label
		}
	}
	pLock.Unlock()
	return false, ""
}
