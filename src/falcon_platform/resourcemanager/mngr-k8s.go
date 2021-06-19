package resourcemanager

import (
	"bytes"
	"context"
	"falcon_platform/logger"
	"io"
	"io/ioutil"
	"k8s.io/apimachinery/pkg/api/meta"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/apis/meta/v1/unstructured"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/runtime/serializer/yaml"
	yamlutil "k8s.io/apimachinery/pkg/util/yaml"
	"k8s.io/client-go/dynamic"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/rest"
	"k8s.io/client-go/restmapper"
	"k8s.io/client-go/tools/clientcmd"
)

type K8sManager struct {
	k8sConfig *rest.Config
}

func InitK8sManager(inCluster bool, kubeconfig string) *K8sManager {

	km := K8sManager{}

	var err error
	// not in-cluster means the km init related code is running not inside the pod
	if !inCluster {

		km.k8sConfig, err = clientcmd.BuildConfigFromFlags("", kubeconfig)
		if err != nil {
			logger.Log.Fatal(err)
		}
	} else {
		// in-cluster means the code is running inside the pod
		// load in-cluster configuration,
		// KUBERNETES_SERVICE_HOST and KUBERNETES_SERVICE_PORT must be defined
		// define them in env variable later

		km.k8sConfig, err = rest.InClusterConfig()
		if err != nil {
			panic(err.Error())
		}
	}

	return &km

}

func (km *K8sManager) CreateResources(filename string) {

	// 1. read file as bytes
	b, err := ioutil.ReadFile(filename)
	if err != nil {
		logger.Log.Fatal(err)
	}
	// print the yaml file to watch
	//logger.Log.Println(string(b))

	// 2. create k8s clients instance
	c, err := kubernetes.NewForConfig(km.k8sConfig)
	if err != nil {
		logger.Log.Fatal(err)
	}

	dd, err := dynamic.NewForConfig(km.k8sConfig)
	if err != nil {
		logger.Log.Fatal(err)
	}

	// 3. decode the file, get multi objs,
	decoder := yamlutil.NewYAMLOrJSONDecoder(bytes.NewReader(b), 300)

	// 4. for each object, create
	for {
		var rawObj runtime.RawExtension
		// can not find any resources in single yaml file, then break out the loop
		if err = decoder.Decode(&rawObj); err != nil {
			break
		}

		obj, gvk, err := yaml.NewDecodingSerializer(unstructured.UnstructuredJSONScheme).Decode(rawObj.Raw, nil, nil)
		unstructuredMap, err := runtime.DefaultUnstructuredConverter.ToUnstructured(obj)
		if err != nil {
			logger.Log.Fatal(err)
		}

		unstructuredObj := &unstructured.Unstructured{Object: unstructuredMap}

		logger.Log.Printf("K8sManager: Creating resources, Name: %s, Kind: %s ...\n",
			unstructuredObj.GetName(),
			unstructuredObj.GetKind())

		gr, err := restmapper.GetAPIGroupResources(c.Discovery())
		if err != nil {
			logger.Log.Fatal(err)
		}

		mapper := restmapper.NewDiscoveryRESTMapper(gr)
		mapping, err := mapper.RESTMapping(gvk.GroupKind(), gvk.Version)
		if err != nil {
			logger.Log.Fatal(err)
		}

		var dri dynamic.ResourceInterface

		if mapping.Scope.Name() == meta.RESTScopeNameNamespace {
			if unstructuredObj.GetNamespace() == "" {
				unstructuredObj.SetNamespace("default")
			}
			dri = dd.Resource(mapping.Resource).Namespace(unstructuredObj.GetNamespace())
		} else {
			dri = dd.Resource(mapping.Resource)
		}

		if _, err := dri.Create(context.Background(), unstructuredObj, metav1.CreateOptions{}); err != nil {
			//logger.Log.Fatal(err)
			logger.Log.Println("K8sManager: Creating resource Error", err)
		}
	}
	if err != io.EOF {
		logger.Log.Println("K8sManager: eof ", err)
	}
}

func (km *K8sManager) UpdateYaml(command string) {
	e := ExecuteBash(command)
	if e != nil {
		panic("K8sManager: MasterError " + e.Error())
	}
}

func (km *K8sManager) DeleteService(svcName string) {
	logger.Log.Printf("K8sManager: deleting svc %s...\n", svcName)

	client, _ := kubernetes.NewForConfig(km.k8sConfig)

	err := client.CoreV1().Services("default").Delete(context.TODO(), svcName, metav1.DeleteOptions{})

	if err != nil {
		logger.Log.Printf("K8sManager: delete svc %s error %s \n", svcName, err)

	}
}
