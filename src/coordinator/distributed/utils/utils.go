package utils

import (
	"log"
	"net"
)

// get one port
func GetFreePort() (int, error) {
	addr, err := net.ResolveTCPAddr("tcp", "localhost:0")
	if err != nil {
		return 0, err
	}

	l, err := net.ListenTCP("tcp", addr)
	if err != nil {
		return 0, err
	}
	defer log.Println(l.Close())
	return l.Addr().(*net.TCPAddr).Port, nil
}

// get many ports
func GetFreePorts(count int) ([]int, error) {
	var ports []int
	for i := 0; i < count; i++ {
		addr, err := net.ResolveTCPAddr("tcp", "localhost:0")
		if err != nil {
			return nil, err
		}

		l, err := net.ListenTCP("tcp", addr)
		if err != nil {
			return nil, err
		}
		ports = append(ports, l.Addr().(*net.TCPAddr).Port)
		log.Println(l.Close())
	}
	return ports, nil
}
