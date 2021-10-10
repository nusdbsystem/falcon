package utils

import (
	"falcon_platform/common"
	"io/ioutil"
)

func ReadFile(path string) (string, error) {
	f, err := ioutil.ReadFile(path)
	if err != nil {
		return "", err
	}
	return string(f), nil
}

func WriteFile(strTest string, fileName string) error {
	var d = []byte(strTest)
	err := ioutil.WriteFile(fileName, d, 0666)
	return err
}

func MinV(a []common.PortType) common.PortType {
	cur := a[0]

	for _, v := range a {
		if cur > v {
			cur = v
		}
	}
	return cur
}

func MaxV(a []common.PortType) common.PortType {
	cur := a[0]

	for _, v := range a {
		if cur < v {
			cur = v
		}
	}
	return cur
}
