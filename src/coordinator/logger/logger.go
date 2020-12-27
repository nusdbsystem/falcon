package logger

import (
	"io"
	"log"
	"os"
	"runtime"
)

var Do *log.Logger
var F *os.File

func GetLogger(fileName string) (*log.Logger, *os.File) {

	f, err := os.OpenFile(fileName, os.O_WRONLY|os.O_CREATE|os.O_APPEND, 0644)
	if err != nil {
		log.Println("Fail to find", fileName, "GetLogger start Failed")
		os.Exit(1)
	}

	writers := []io.Writer{
		f,
		os.Stdout}

	fileAndStdoutWriter := io.MultiWriter(writers...)

	// use log.Llongfile to display full path
	// logger := log.New(fileAndStdoutWriter, "", log.Ldate|log.Ltime|log.Lshortfile)
	logger := log.New(fileAndStdoutWriter, "[log]\t", log.Llongfile)

	return logger, f

}

func HandleErrors() {
	// cache global unexpected error
	err := recover()
	if err != nil {
		Do.Printf("[Error]\t")
		Do.Println("HandleErrors: Catching error ...")
		var buf [4096]byte
		n := runtime.Stack(buf[:], false)
		Do.Println(err)
		Do.Printf("==> %s\n", string(buf[:n]))
		//os.Exit(1)
	}
	Do.Println("HandleErrors: exit current thread")
}
