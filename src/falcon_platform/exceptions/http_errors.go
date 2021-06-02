package exceptions

import (
	"falcon_platform/logger"
	"fmt"
	"net/http"
)

func HandlePanic(w http.ResponseWriter, r *http.Request) {
	err := recover()
	if err != nil {
		errstr := fmt.Sprintf("%s", err)
		HandleHttpError(w, r, http.StatusInternalServerError, errstr)
	}
}

func HandleHttpError(w http.ResponseWriter, r *http.Request, statusCode int, errMsg string) {
	logger.Log.Printf("HTTP Error (%d): Processing %s, Error: %s\n", statusCode, r.RequestURI, errMsg)
	w.WriteHeader(statusCode)
	w.Write([]byte(http.StatusText(statusCode) + "\n"))
	w.Write([]byte(errMsg + "\n"))
}
