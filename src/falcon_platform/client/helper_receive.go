package client

import (
	"bytes"
	"falcon_platform/logger"
	"io"
	"net/http"
)

// send by file=
func ReceiveFile(r *http.Request, buf bytes.Buffer, key string) (error, string) {
	// FormFile returns the first file for the given key
	// it also returns the FileHeader so we can get the Filename,
	// the Header and the size of the file
	file, handler, err := r.FormFile(key)
	if err != nil {
		return err, ""
	}
	defer file.Close()

	logger.Log.Printf("Uploaded File: %+v\n", handler.Filename)
	logger.Log.Printf("File Size: %+v\n", handler.Size)
	logger.Log.Printf("MIME Header: %+v\n", handler.Header)

	_, e := io.Copy(&buf, file)
	if e != nil {
		logger.Log.Println("copy error", e)
	}

	return nil, buf.String()
}

// send by data=
func ReceiveForm(r *http.Request) {
	if err := r.ParseForm(); err != nil {
		logger.Log.Printf("ParseForm() err: %v", err)
	}
	//logger.Log.Printf( "PostFrom = %v\n", r.PostForm)

	//name := r.FormValue("name")
	//addr := r.FormValue("addr")
}
