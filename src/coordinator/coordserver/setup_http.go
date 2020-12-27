package coordserver

import (
	"context"
	"coordinator/common"
	"coordinator/coordserver/controller"
	md "coordinator/coordserver/middleware"
	rt "coordinator/coordserver/router"
	"coordinator/logger"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"
)

type key int

const (
	requestIDKey key = 0
)

func hello(w http.ResponseWriter, req *http.Request) {
	w.WriteHeader(http.StatusOK)
	fmt.Fprintf(w, "hello from falcon coordinator\n")
}

func SetupHttp(nConsumer int) {
	defer logger.HandleErrors()
	mux := http.NewServeMux()

	md.SysLvPath = []string{common.Register, common.PartyServerAdd}

	// sanity check
	mux.HandleFunc("/", hello)

	//job
	mux.HandleFunc("/"+common.SubmitJob, md.AddRouter(rt.JobSubmit, http.MethodPost))
	mux.HandleFunc("/"+common.StopJob, md.AddRouter(rt.JobKill, http.MethodPost))
	mux.HandleFunc("/"+common.UpdateTrainJobMaster, md.AddRouter(rt.JobUpdateMaster, http.MethodPost))
	mux.HandleFunc("/"+common.UpdateJobStatus, md.AddRouter(rt.JobUpdateStatus, http.MethodPost))
	mux.HandleFunc("/"+common.UpdateJobResInfo, md.AddRouter(rt.JobUpdateResInfo, http.MethodPost))

	mux.HandleFunc("/"+common.QueryJobStatus, md.AddRouter(rt.JobStatusQuery, http.MethodGet))

	//party server
	mux.HandleFunc("/"+common.Register, md.AddRouter(rt.UserRegister, http.MethodPost))
	mux.HandleFunc("/"+common.PartyServerAdd, md.AddRouter(rt.PartyServerAdd, http.MethodPost))
	mux.HandleFunc("/"+common.PartyServerDelete, md.AddRouter(rt.PartyServerDelete, http.MethodPost))
	mux.HandleFunc("/"+common.GetPartyServerPort, md.AddRouter(rt.GetPartyServerPort, http.MethodGet))

	// model serving
	mux.HandleFunc("/"+common.ModelUpdate, md.AddRouter(rt.ModelUpdate, http.MethodPost))

	// prediction service
	mux.HandleFunc("/"+common.UpdateInferenceJobMaster, md.AddRouter(rt.InferenceUpdateMaster, http.MethodPost))
	mux.HandleFunc("/"+common.InferenceUpdate, md.AddRouter(rt.UpdateInference, http.MethodPost))
	mux.HandleFunc("/"+common.InferenceCreate, md.AddRouter(rt.CreateInference, http.MethodPost))
	mux.HandleFunc("/"+common.InferenceStatusUpdate, md.AddRouter(rt.UpdateInferenceStatus, http.MethodPost))

	// resource
	mux.HandleFunc("/"+common.AssignPort, md.AddRouter(rt.AssignPort, http.MethodGet))
	mux.HandleFunc("/"+common.AddPort, md.AddRouter(rt.AddPort, http.MethodPost))

	// for logging and tracing
	nextRequestID := func() string {
		return fmt.Sprintf("%d", time.Now().UnixNano())
	}
	http_logger := log.New(os.Stdout, "http: ", log.LstdFlags)

	// run
	server := &http.Server{
		Addr:    common.CoordAddr,
		Handler: tracing(nextRequestID)(logging(http_logger)(mux)),
	}

	logger.Do.Printf(
		"[coordinator server] listening on IP: %v, Port: %v\n",
		common.CoordIP,
		common.CoordPort)

	logger.Do.Println("HTTP: Updating table...")
	controller.CreateTables()
	controller.CreateSysPorts()

	logger.Do.Println("HTTP: Creating admin user...")
	controller.CreateUser()

	jobScheduler := controller.Init(nConsumer)

	done := make(chan os.Signal)
	signal.Notify(done, os.Interrupt, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		<-done

		logger.Do.Println("HTTP: Stop multi consumers")

		jobScheduler.StopMonitor()
		logger.Do.Println("HTTP: Monitor Stopped")

		for i := 0; i < nConsumer; i++ {
			jobScheduler.StopConsumer()
		}

		//todo, this will shutdown the master thread at the same time
		// but the worker need to be stopped also??, add later ???

		logger.Do.Println("HTTP: Consumer Stopped")
		if err := server.Shutdown(context.Background()); err != nil {
			logger.Do.Fatal("HTTP: ShutDown the server", err)
		}
	}()

	logger.Do.Println("HTTP: Starting multi consumers...")

	// multi-thread consumer

	for i := 0; i < nConsumer; i++ {

		go jobScheduler.Consume(i)
	}
	go jobScheduler.MonitorConsumers()

	logger.Do.Println("HTTP: Starting HTTP server...")
	err := server.ListenAndServe()

	if err != nil {
		if err == http.ErrServerClosed {
			logger.Do.Print("HTTP: Server closed under request\n", err)
		} else {
			logger.Do.Fatal("HTTP: Server closed unexpected\n", err)
		}
	}

}

// logging of http requests by https://gist.github.com/enricofoltran/10b4a980cd07cb02836f70a4ab3e72d7
func logging(logger *log.Logger) func(http.Handler) http.Handler {
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
func tracing(nextRequestID func() string) func(http.Handler) http.Handler {
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
