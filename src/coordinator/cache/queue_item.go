package cache

import "coordinator/common"

type QItem struct {
	IPs       []string
	JobId     uint
	PartyPath []common.PartyPath
	TaskInfos common.Tasks

	ModelPath []string
	ExecutablePath []string
}
