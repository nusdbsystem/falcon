package worker

import (
	"falcon_platform/logger"
)

func (wk *InferenceWorker) CreateInference(_ string) {
	// todo gobuild.sh sub process to run prediction job

	logger.Log.Println("InferenceWorker: CreateService")

	//cmd := exec.Command("python3", "/go/preprocessing.py", "-a=1", "-b=2")

	// 2 thread will ready from isStop channel, only one is running at the any time

	//wk.Tm.CreateResources(cmd)

}

func (wk *InferenceWorker) UpdateInference() error {
	return nil
}

func (wk *InferenceWorker) QueryInference() error {
	return nil
}
