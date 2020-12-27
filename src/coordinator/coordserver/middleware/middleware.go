package middleware

import (
	"coordinator/coordserver/entity"
	"coordinator/logger"
	"net/http"
	"time"
)

type Middleware func(handler http.HandlerFunc) http.HandlerFunc

var SysLvPath []string

// middleware used to measure time
func timeUsage() Middleware {
	return func(f http.HandlerFunc) http.HandlerFunc {

		return func(w http.ResponseWriter, r *http.Request) {
			start := time.Now()
			defer func() {
				logger.Do.Printf("HTTP: [url] %s [time_usage] %s \n", r.Host+r.URL.Path, time.Since(start))
			}()
			f(w, r)
		}
	}
}

// middleware used to verify the methods
func methodVerify(method string) Middleware {
	return func(f http.HandlerFunc) http.HandlerFunc {

		return func(w http.ResponseWriter, r *http.Request) {
			if method != r.Method {
				http.Error(w, "HTTP: Method not correct", http.StatusBadRequest)
				return
			}
			//logger.Do.Println("HTTP: Checking method")
			f(w, r)
		}
	}
}

// middleware used to verify the methods
func handPanic() Middleware {
	return func(f http.HandlerFunc) http.HandlerFunc {

		return func(w http.ResponseWriter, r *http.Request) {
			defer handlePanic(w, r)
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

	// change f to newF,

	newF := InitContext(f, SysLvPath)

	// defaultMiddleWare := []Middleware{handPanic(), methodVerify(Method), timeUsage()}
	// disable timeUsage
	defaultMiddleWare := []Middleware{handPanic(), methodVerify(Method)}

	//defaultMiddleWare = append(defaultMiddleWare, middleWares...)

	for _, middleWare := range defaultMiddleWare {
		newF = middleWare(newF)
	}
	return newF
}
