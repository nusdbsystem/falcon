package middleware

import (
	"coordinator/api/entity"
	"fmt"
	"net/http"
	"time"
)

type Middleware func(handler http.HandlerFunc) http.HandlerFunc

var SysLvPath []string

// middle ware used to measure time
func timeUsage() Middleware {
	return func(f http.HandlerFunc) http.HandlerFunc {

		return func(w http.ResponseWriter, r *http.Request) {
			start := time.Now()
			defer func() {
				fmt.Println("HTTP: Time of process func:", r.URL.Path, "is:", time.Since(start))
			}()
			//fmt.Println("HTTP: Checking time")
			f(w, r)
		}
	}
}

// middle ware used to verify the methods
func methodVerify(method string) Middleware {
	return func(f http.HandlerFunc) http.HandlerFunc {

		return func(w http.ResponseWriter, r *http.Request) {
			if method != r.Method {
				http.Error(w, "HTTP: Method not correct", http.StatusBadRequest)
				return
			}
			//fmt.Println("HTTP: Checking method")
			f(w, r)
		}
	}
}

// middle ware used to verify the methods
func handPanic() Middleware {
	return func(f http.HandlerFunc) http.HandlerFunc {

		return func(w http.ResponseWriter, r *http.Request) {
			defer handlePanic(w)
			f(w, r)
		}
	}
}

// user level router,
func AddRouter(
	f func(w http.ResponseWriter, r *http.Request, ctx *entity.Context),
	Method string,
	middleWares ...Middleware) http.HandlerFunc {

	// change f to newF,

	newF := InitContext(f, SysLvPath)

	defaultMiddleWare := []Middleware{handPanic(), methodVerify(Method), timeUsage()}

	defaultMiddleWare = append(defaultMiddleWare, middleWares...)

	for _, middleWare := range defaultMiddleWare {
		newF = middleWare(newF)
	}
	return newF
}
