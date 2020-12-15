package middleware

import (
	"coordinator/logger"
	"fmt"
	"net/http"
)

func handlePanic(w http.ResponseWriter, r *http.Request) {
	err := recover()
	if err != nil {
		logger.Do.Printf("HTTP: Processing %s Error: %s \n",r.RequestURI, err)
		fmt.Printf("HTTP: Processing %s Error: %s \n",r.RequestURI, err)
		errstr := fmt.Sprintf("%s", err)
		_, _ = w.Write([]byte(errstr))
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusInternalServerError)
	}
}
