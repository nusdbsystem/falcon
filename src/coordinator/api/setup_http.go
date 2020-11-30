package api

import (
	"context"
	"coordinator/api/controller"
	md "coordinator/api/middleware"
	rt "coordinator/api/router"
	"coordinator/config"
	"coordinator/logger"
	"net/http"
	"os"
	"os/signal"
	"syscall"
)

func handlePanic() {
	err := recover()
	if err != nil {
		logger.Do.Println(err)
	}
}
func SetupHttp(host, port string, nConsumer int) {
	defer handlePanic()
	httpAddr := host + ":" + port
	mux := http.NewServeMux()

	md.SysLvPath = []string{config.Register, config.ListenerAdd}

	//job post
	mux.HandleFunc("/"+config.SubmitJob, md.AddRouter(rt.JobSubmit, http.MethodPost))
	mux.HandleFunc("/"+config.StopJob, md.AddRouter(rt.JobKill, http.MethodPost))
	mux.HandleFunc("/"+config.UpdateJobMaster, md.AddRouter(rt.JobUpdateMaster, http.MethodPost))
	mux.HandleFunc("/"+config.UpdateJobStatus, md.AddRouter(rt.JobUpdateStatus, http.MethodPost))
	mux.HandleFunc("/"+config.UpdateJobResInfo, md.AddRouter(rt.JobUpdateResInfo, http.MethodPost))

	//job post
	mux.HandleFunc("/"+config.QueryJobStatus, md.AddRouter(rt.JobStatusQuery, http.MethodGet))

	//listener
	mux.HandleFunc("/"+config.Register, md.AddRouter(rt.UserRegister, http.MethodPost))
	mux.HandleFunc("/"+config.ListenerAdd, md.AddRouter(rt.ListenerAdd, http.MethodPost))
	mux.HandleFunc("/"+config.ListenerDelete, md.AddRouter(rt.ListenerDelete, http.MethodPost))

	// model serving
	mux.HandleFunc("/"+config.ModelUpdate, md.AddRouter(rt.ModelUpdate, http.MethodPost))
	mux.HandleFunc("/"+config.SvcPublishing, md.AddRouter(rt.PublishService, http.MethodPost))
	mux.HandleFunc("/"+config.SvcCreate, md.AddRouter(rt.CreateService, http.MethodPost, host, port))
	mux.HandleFunc("/"+config.UpdateModelServiceStatus, md.AddRouter(rt.ModelServiceUpdateStatus, http.MethodPost))


	server := &http.Server{
		Addr:    httpAddr,
		Handler: mux,
	}

	logger.Do.Println("HTTP: Updating table...")
	controller.CreateTables()

	logger.Do.Println("HTTP: Creating admin user...")
	controller.CreateUser()

	dslScheduler := controller.Init(host, port, nConsumer)

	done := make(chan os.Signal)
	signal.Notify(done, os.Interrupt, syscall.SIGINT, syscall.SIGTERM)
	go func() {

		<-done

		logger.Do.Println("HTTP: Stop multi consumers")

		dslScheduler.StopMonitor()
		logger.Do.Println("HTTP: Monitor Stopped")

		for i := 0; i < nConsumer; i++ {
			dslScheduler.StopConsumer()
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

		go dslScheduler.Consume(i)
	}
	go dslScheduler.MonitorConsumers()

	logger.Do.Println("HTTP: Starting HTTP server...")
	err := server.ListenAndServe()

	if err != nil {
		if err == http.ErrServerClosed {
			logger.Do.Print("HTTP: Server closed under request", err)
		} else {
			logger.Do.Fatal("HTTP: Server closed unexpected", err)
		}
	}

}
