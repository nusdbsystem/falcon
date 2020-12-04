package test

import (
	"coordinator/distributed/taskmanager"
	"testing"
)

func Test3(t *testing.T) {

	kubeconfig :=  "/Users/nailixing/.kube/common"
	filename := "/Users/nailixing/GOProj/src/github.com/falcon/src/coordinator/test/test.yaml"

	km := taskmanager.InitK8sManager(false, kubeconfig)
	km.CreateResources(filename)
}