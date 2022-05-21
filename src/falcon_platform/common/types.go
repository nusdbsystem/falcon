/**
 * @Description: define some variable type, for consistent
 * @File:  types
 * @Version: 1.0.0
 * @Date: 24/08/21 下午10:33
 */
package common

type PartyIdType uint

type WorkerIdType uint

type PortType int32

// ConvertPartyType2Int 1. convert party-type to int, partyType 1=Passive, 0=Active
func ConvertPartyType2Int(partyTypeStr string) int {
	var partyType int
	if partyTypeStr == "active" {
		partyType = 0
	} else if partyTypeStr == "passive" {
		partyType = 1
	}
	return partyType
}

// ConvertJobFlType2Int 2. convert fl-type to int, fl-setting 1=vertical, 0=horizontal
func ConvertJobFlType2Int(flSettingStr string) int {
	flSetting := 1 // default vertical
	if flSettingStr == "vertical" {
		flSetting = 1
	} else if flSettingStr == "horizontal" {
		flSetting = 0
	}
	return flSetting
}
