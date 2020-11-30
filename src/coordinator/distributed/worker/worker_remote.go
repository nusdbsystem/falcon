package worker

import (
	"coordinator/logger"
	"fmt"
	"golang.org/x/crypto/ssh"
	"io/ioutil"
	"net"
	"time"
)

const (
	CERT_PASSWORD        = 1
	CERT_PUBLIC_KEY_FILE = 2
	DEFAULT_TIMEOUT      = 3 // second
)

type SSH struct {
	Ip      string
	User    string
	Cert    string //password or key file path
	Port    int
	session *ssh.Session
	client  *ssh.Client
}

func (ssh_client *SSH) readPublicKeyFile(file string) ssh.AuthMethod {
	buffer, err := ioutil.ReadFile(file)
	if err != nil {
		return nil
	}
	key, err := ssh.ParsePrivateKey(buffer)
	if err != nil {
		return nil
	}
	return ssh.PublicKeys(key)
}

func (ssh_client *SSH) Connect(mode int) {

	var ssh_config *ssh.ClientConfig
	var auth []ssh.AuthMethod
	if mode == CERT_PASSWORD {
		auth = []ssh.AuthMethod{ssh.Password(ssh_client.Cert)}
	} else if mode == CERT_PUBLIC_KEY_FILE {
		auth = []ssh.AuthMethod{ssh_client.readPublicKeyFile(ssh_client.Cert)}
	} else {
		logger.Do.Println("does not support mode: ", mode)
		return
	}

	ssh_config = &ssh.ClientConfig{
		User: ssh_client.User,
		Auth: auth,
		//需要验证服务端，不做验证返回nil就可以，点击HostKeyCallback看源码就知道了
		HostKeyCallback: func(hostname string, remote net.Addr, key ssh.PublicKey) error {
			return nil
		},
		Timeout: time.Second * DEFAULT_TIMEOUT,
	}

	client, err := ssh.Dial("tcp", fmt.Sprintf("%s:%d", ssh_client.Ip, ssh_client.Port), ssh_config)
	if err != nil {
		logger.Do.Println(err)
		return
	}

	session, err := client.NewSession()
	if err != nil {
		logger.Do.Println(err)
		_ = client.Close()
		return
	}

	ssh_client.session = session
	ssh_client.client = client
}

func (ssh_client *SSH) RunCmd(cmd string) {
	out, err := ssh_client.session.CombinedOutput(cmd)
	if err != nil {
		logger.Do.Println(err)
	}
	logger.Do.Println(string(out))
}

func (ssh_client *SSH) Close() {
	_ = ssh_client.session.Close()
	_ = ssh_client.client.Close()
}

func main() {
	client := &SSH{
		Ip:   "panda.d2.comp.nus.edu.sg",
		User: "xingnaili",
		Port: 22,
		Cert: "",
	}
	client.Connect(CERT_PASSWORD)
	client.RunCmd("python testRemoteGo.py")
	client.Close()
}
