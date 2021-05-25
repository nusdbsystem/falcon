package utils

import (
	"coordinator/logger"
	"fmt"
	"net/http"
)

func HandlePanic(w http.ResponseWriter, r *http.Request) {
	err := recover()
	if err != nil {
		logger.Log.Printf("HTTP Error Handler: Processing %s Error: %s \n", r.RequestURI, err)
		errstr := fmt.Sprintf("%s", err)
		w.WriteHeader(http.StatusInternalServerError)
		w.Write([]byte(errstr))
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
	}
}
