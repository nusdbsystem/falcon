package exceptions

import (
	"falcon_platform/logger"
	"fmt"
	"net/http"
)

func HandlePanic(w http.ResponseWriter, r *http.Request) {
	err := recover()
	if err != nil {
		errStr := fmt.Sprintf("%s", err)
		HandleHttpError(w, r, http.StatusInternalServerError, errStr)
	} else {
		// return result as json string,
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)
	}
}

func HandleHttpError(w http.ResponseWriter, r *http.Request, statusCode int, errMsg string) {
	logger.Log.Printf("HTTP Error (%d): Processing %s, Error: %s\n", statusCode, r.RequestURI, errMsg)
	w.WriteHeader(statusCode)
	_, _ = w.Write([]byte(http.StatusText(statusCode) + "\n"))
	_, _ = w.Write([]byte(errMsg + "\n"))
}
