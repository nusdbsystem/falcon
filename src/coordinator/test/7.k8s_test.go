package test

import (
	"fmt"
	"math/rand"
	"testing"
	"time"
)

func Test3(t *testing.T) {

	//kubeconfig :=  "/Users/nailixing/.kube/config"
	//filename := "/Users/nailixing/GOProj/src/github.com/falcon/src/coordinator/deploy/template/test.yaml"
	//
	//km := taskmanager.InitK8sManager(false, kubeconfig)
	//km.CreateResources(filename)

	i := 0
	for {

		if i==10{
			break
		}
		i++
		r := rand.New(rand.NewSource(time.Now().Unix()))
		fmt.Println(r.Int())
		res := fmt.Sprintf("%d", r.Int())
		fmt.Println(res)
		fmt.Println("----------------------------------------")
		time.Sleep(time.Second*1)
	}

}