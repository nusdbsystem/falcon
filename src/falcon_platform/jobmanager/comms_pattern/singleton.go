package comms_pattern

import (
	"sync"
)

// partyNetCfg Global var,
var partyNetCfgOnce sync.Once
var partyNetCfg map[string]PartyNetworkConfig

func GetAllNetworkCfg() map[string]PartyNetworkConfig {
	partyNetCfgOnce.Do(func() {
		partyNetCfg = make(map[string]PartyNetworkConfig)
	})
	return partyNetCfg
}

// jobNetCfg Global var,
var jobNetCfgOnce sync.Once
var jobNetCfg map[string]JobNetworkConfig

func GetJobNetCfg() map[string]JobNetworkConfig {
	jobNetCfgOnce.Do(func() {
		jobNetCfg = make(map[string]JobNetworkConfig)
	})
	return jobNetCfg
}
