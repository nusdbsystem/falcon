package prediction

import (
	"coordinator/distributed/base"
)

type ModelService struct{
	base.RpcBase

}


func (msvc *ModelService) CreateService() error {
	return nil
}


func (msvc *ModelService) LaunchService() error {
	return nil
}


func (msvc *ModelService) UpdateService() error {
	return nil
}

func (msvc *ModelService) QueryService() error {
	return nil
}

func (msvc *ModelService) StopService() error {
	return nil
}

func (msvc *ModelService) DeleteService() error {
	return nil
}
