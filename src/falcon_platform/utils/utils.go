package utils

import (
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
