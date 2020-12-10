package listener

import (
	"context"
	c "coordinator/client"
	"coordinator/common"
	rt "coordinator/listener/router"
	"coordinator/logger"
	"net/http"
	"os"
	"time"
)

func SetupListener() {
	// host: listenerAddr
	// ServerAddress: the address for main http server
	// host port:  for listener,
	defer logger.HandleErrors()
	mux := http.NewServeMux()

	mux.HandleFunc("/"+common.SetupWorker, rt.SetupWorker())
	logger.Do.Println("SetupListener: registering listenerPort to coord", common.ListenerPort)

	server := &http.Server{
		Addr:    "0.0.0.0:" + common.ListenerPort,
		Handler: mux,
	}
	// report address to flow htp server
	done := make(chan os.Signal)

	go func() {

		<-done
		if err := server.Shutdown(context.Background()); err != nil {
			logger.Do.Fatal("ShutDown the server", err)
		}

		c.ListenerDelete(common.CoordSvcURLGlobal, common.ListenAddrGlobal)
	}()

	NTimes := 20
	for {
		if NTimes<0{
			panic("\"SetupListener: connecting to coord Db...retry\"")
		}
		err := c.AddPort(common.CoordSvcURLGlobal, common.ListenerPort)
		if err != nil{
			logger.Do.Println(err)
			logger.Do.Printf("SetupListener: connecting to coord %s ...retry \n", common.CoordSvcURLGlobal)
			time.Sleep(time.Second*5)
			NTimes--
		}else{
			break
		}
	}

	NTimes = 20
	for {
		if NTimes<0{
			panic("\"SetupListener: connecting to coord Db...retry\"")
		}
		err := c.ListenerAdd(common.CoordSvcURLGlobal, common.ListenAddrGlobal, common.ListenerPort)

		if err != nil{
			logger.Do.Println(err)
			logger.Do.Printf("SetupListener: ListenerAdd %s ...retry \n", common.ListenAddrGlobal)
			time.Sleep(time.Second*5)
			NTimes--
		}else{
			break
		}
	}



	logger.Do.Println("Starting HTTP server...")
	err := server.ListenAndServe()

	if err != nil {
		if err == http.ErrServerClosed {
			logger.Do.Print("Server closed under request ", err)
		} else {
			logger.Do.Fatal("Server closed unexpected ", err)
		}
	}
}
