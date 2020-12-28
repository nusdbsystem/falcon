package common

import (
	"context"
	"fmt"
	"log"
	"net/http"
	"time"
)

type key int

const (
	requestIDKey key = 0
)

func HelloCoordinator(w http.ResponseWriter, req *http.Request) {
	w.WriteHeader(http.StatusOK)
	fmt.Fprintf(w, "hello from falcon coordinator\n")
}

func HelloPartyServer(w http.ResponseWriter, req *http.Request) {
	w.WriteHeader(http.StatusOK)
	fmt.Fprintf(w, "hello from falcon party server~\n")
}

// for logging and tracing
func NextRequestID() string {
	return fmt.Sprintf("%d", time.Now().UnixNano())
}

// logging of http requests by https://gist.github.com/enricofoltran/10b4a980cd07cb02836f70a4ab3e72d7
func Logging(logger *log.Logger) func(http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			defer func() {
				requestID, ok := r.Context().Value(requestIDKey).(string)
				if !ok {
					requestID = "unknown"
				}
				logger.Println(requestID[:3], r.Method, r.URL.Path, r.RemoteAddr, r.UserAgent())
			}()
			next.ServeHTTP(w, r)
		})
	}
}

// tracing and logging by https://gist.github.com/enricofoltran/10b4a980cd07cb02836f70a4ab3e72d7
func Tracing(nextRequestID func() string) func(http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			requestID := r.Header.Get("X-Request-Id")
			if requestID == "" {
				requestID = nextRequestID()
			}
			ctx := context.WithValue(r.Context(), requestIDKey, requestID)
			w.Header().Set("X-Request-Id", requestID)
			next.ServeHTTP(w, r.WithContext(ctx))
		})
	}
}
