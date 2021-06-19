package test

import (
	"falcon_platform/resourcemanager"
	"fmt"
	"testing"
)

func TestDist(t *testing.T) {

	res, _ := resourcemanager.GetFreePort4K8s()
	res2, _ := resourcemanager.GetFreePorts(3)
	fmt.Println(res, res2)

}
