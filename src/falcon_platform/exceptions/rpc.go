package exceptions

import "errors"




const (

	ConnectionErr = "1"
	CallingErr = "2"

)


func ConnectionError() error {
	return errors.New(ConnectionErr)
}

func CallingError() error {
	return errors.New(CallingErr)
}