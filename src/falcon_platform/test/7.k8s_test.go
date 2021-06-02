package test

import (
	"context"
	"fmt"
	"log"
	"testing"

	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/tools/clientcmd"
)

func Test3(t *testing.T) {

	//kubeconfig :=  "/Users/nailixing/.kube/config"
	//filename := "/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/deploy/template/test.yaml"
	//
	//km := taskmanager.InitK8sManager(false, kubeconfig)
	//km.CreateResources(filename)

	//i := 0
	//for {
	//
	//	if i==10{
	//		break
	//	}
	//	i++
	//	r := rand.New(rand.NewSource(time.Now().Unix()))
	//	fmt.Println(r.Int())
	//	res := fmt.Sprintf("%d", r.Int())
	//	fmt.Println(res)
	//	fmt.Println("----------------------------------------")
	//	time.Sleep(time.Second*1)
	//}

}

func TestDelete(t *testing.T) {
	kubeconfig := "/Users/nailixing/.kube/config"

	//km.CreateResources(filename)
	k8sConfig, err := clientcmd.BuildConfigFromFlags("", kubeconfig)

	client, _ := kubernetes.NewForConfig(k8sConfig)

	err2 := client.CoreV1().Services("default").Delete(context.TODO(), "my-nginx", metav1.DeleteOptions{})
	if err2 != nil {
		fmt.Println("Errors:  ", err2)
		log.Fatal(err)
	}
}
