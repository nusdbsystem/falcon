package middleware

import (
	"net/http"
)

func handlePanic(w http.ResponseWriter) {
	err := recover()
	if err != nil {
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusInternalServerError)
	}
}
