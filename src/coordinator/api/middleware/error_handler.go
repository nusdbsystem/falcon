package middleware

import (
	"coordinator/logger"
	"net/http"
)

func handlePanic(w http.ResponseWriter, r *http.Request) {
	err := recover()
	if err != nil {
		logger.Do.Printf("HTTP: Processing %s Error: %s \n",r.RequestURI, err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusInternalServerError)
	}
}
