package middleware

import (
	"falcon_platform/coordserver/entity"
	"falcon_platform/coordserver/utils"
	"falcon_platform/logger"
	"net/http"
	"time"
)

type Middleware func(handler http.HandlerFunc) http.HandlerFunc

var SysLvPath []string

// middleware to measure time
func timeUsage() Middleware {
	return func(f http.HandlerFunc) http.HandlerFunc {

		return func(w http.ResponseWriter, r *http.Request) {
			start := time.Now()
			defer func() {
				logger.Log.Printf("HTTP: [url] %s [time_usage] %s \n", r.Host+r.URL.Path, time.Since(start))
			}()
			f(w, r)
		}
	}
}

// middleware to verify the HTTP methods
func verifyHTTPmethod(method string) Middleware {
	return func(f http.HandlerFunc) http.HandlerFunc {

		return func(w http.ResponseWriter, r *http.Request) {
			if method != r.Method {
				http.Error(w, "HTTP: Method not correct", http.StatusBadRequest)
				return
			}
			//logger.Log.Println("HTTP: Checking method")
			f(w, r)
		}
	}
}

// middleware call panic handler
func callPanic() Middleware {
	return func(f http.HandlerFunc) http.HandlerFunc {

		return func(w http.ResponseWriter, r *http.Request) {
			defer utils.HandlePanic(w, r)
			f(w, r)
		}
	}
}

// user level router,
func AddRouter(
	f func(
		w http.ResponseWriter,
		r *http.Request,
		ctx *entity.Context,
	),
	Method string,
) http.HandlerFunc {

	middlewareHandler := InitContext(f, SysLvPath)

	// defaultMiddleWare := []Middleware{callPanic(), verifyHTTPmethod(Method), timeUsage()}
	// disable timeUsage
	defaultMiddleWare := []Middleware{callPanic(), verifyHTTPmethod(Method)}

	//defaultMiddleWare = append(defaultMiddleWare, middleWares...)

	for _, middleWare := range defaultMiddleWare {
		middlewareHandler = middleWare(middlewareHandler)
	}
	return middlewareHandler
}
