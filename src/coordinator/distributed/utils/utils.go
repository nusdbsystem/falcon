package utils

import (
	"coordinator/logger"
	"fmt"
	"math/rand"
	"net"
)

// get one port
func GetFreePort() (int, error) {
	url, err := net.ResolveTCPAddr("tcp", "localhost:0")
	if err != nil {
		return 0, err
	}

	l, err := net.ListenTCP("tcp", url)
	if err != nil {
		return 0, err
	}
	defer logger.Do.Println(l.Close())
	return l.Addr().(*net.TCPAddr).Port, nil
}

// get many ports
func GetFreePorts(count int) ([]int, error) {
	var ports []int
	for i := 0; i < count; i++ {
		url, err := net.ResolveTCPAddr("tcp", "localhost:0")
		if err != nil {
			return nil, err
		}

		l, err := net.ListenTCP("tcp", url)
		if err != nil {
			return nil, err
		}
		ports = append(ports, l.Addr().(*net.TCPAddr).Port)
		logger.Do.Println(l.Close())
	}
	return ports, nil
}


// get one port
func GetFreePort4K8s() (int, error) {
	min := 30000
	max := 32767
	p := rand.Intn(max - min) + min

	var url *net.TCPAddr
	var l *net.TCPListener
	var err error

	for {
		url, err = net.ResolveTCPAddr("tcp", "localhost:"+fmt.Sprintf("%d",p))
		if err != nil {
			return 0, err
		}

		l, err = net.ListenTCP("tcp",url)
		if err != nil {
			p++
		}else{
			logger.Do.Println(l.Close())
			return p, nil
		}
	}
}



func Contains(str string, l []string) bool {
	for _, ls :=  range l{
		if ls == str{
			return true
		}
	}
	return false
}