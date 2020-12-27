package client

import (
	"bytes"
	"coordinator/logger"
	"io"
	"net/http"
	"strings"
)

// send by file=
func ReceiveFile(r *http.Request, buf bytes.Buffer, key string) (error, string) {
	// in your case file would be file upload
	file, header, err := r.FormFile(key)
	if err != nil {
		panic(err)
	}
	defer file.Close()

	name := strings.Split(header.Filename, ".")
	logger.Do.Printf("File name = %s\n", name)
	_, e := io.Copy(&buf, file)
	if e != nil {
		logger.Do.Println("copy error", e)
	}
	contents := buf.String()
	return nil, contents
}

// send by data=
func ReceiveForm(r *http.Request) {
	if err := r.ParseForm(); err != nil {
		logger.Do.Printf("ParseForm() err: %v", err)
	}
	//logger.Do.Printf( "PostFrom = %v\n", r.PostForm)

	//name := r.FormValue("name")
	//addr := r.FormValue("addr")
}

func Tests() {
	panic("math: square root of negative number")
	//return errors.New("math: square root of negative number")
}
