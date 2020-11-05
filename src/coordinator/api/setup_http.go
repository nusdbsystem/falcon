package api

import (
	"context"
	"coordinator/api/controller"
	md "coordinator/api/middleware"
	rt "coordinator/api/router"
	"coordinator/config"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
)

func handlePanic() {
	err := recover()
	fmt.Println(err)
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

	server := &http.Server{
		Addr:    httpAddr,
		Handler: mux,
	}

	fmt.Println("HTTP: Updating table...")
	controller.CreateTables()

	fmt.Println("HTTP: Creating admin user...")
	controller.CreateUser()

	dslScheduler := controller.Init(host, port, nConsumer)

	done := make(chan os.Signal)
	signal.Notify(done, os.Interrupt, syscall.SIGINT, syscall.SIGTERM)
	go func() {

		<-done

		fmt.Println("HTTP: Stopping muti consumers")

		dslScheduler.StopMonitor()
		fmt.Println("HTTP: Monitor Stopped")

		for i := 0; i < nConsumer; i++ {
			dslScheduler.StopConsumer()
		}

		//todo, this will shutdown the master thread at the same time
		// but the worker need to be stopped also??, add later ???

		fmt.Println("HTTP: Consumer Stopped")
		if err := server.Shutdown(context.Background()); err != nil {
			log.Fatal("HTTP: ShutDown the server", err)
		}
	}()

	fmt.Println("HTTP: Starting muti consumers...")

	// multi-thread consumer

	for i := 0; i < nConsumer; i++ {

		go dslScheduler.Consume(i)
	}
	go dslScheduler.MonitorConsumers()

	fmt.Println("HTTP: Starting HTTP server...")
	err := server.ListenAndServe()

	if err != nil {
		if err == http.ErrServerClosed {
			log.Print("HTTP: Server closed under request", err)
		} else {
			log.Fatal("HTTP: Server closed unexpected", err)
		}
	}

}
