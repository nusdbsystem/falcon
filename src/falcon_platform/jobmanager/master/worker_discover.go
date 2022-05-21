package master

import (
	"bytes"
	"encoding/json"
	"falcon_platform/jobmanager/entity"
)

// RegisterWorker is an RPC method that is called by workers after they have started
// up to report that they are ready to receive tasks.
func (master *Master) RegisterWorker(args *entity.WorkerInfo, _ *struct{}) error {

	var out bytes.Buffer
	bs, _ := json.Marshal(args)
	_ = json.Indent(&out, bs, "", "\t")
	master.Logger.Printf("[master.RegisterWorker] one Worker registered! args: %v\n", out.String())

	// Pass WorkerAddrIdType (addr:partyID) into tmpWorkers for pre-processing
	// IP:Port:WorkerID
	encodedStr := entity.EncodeWorkerInfo(args)
	master.tmpWorkers <- encodedStr
	return nil
}

// sends information of worker to ch. which is used by scheduler
func (master *Master) forwardRegistrations() {
	master.Lock()
	master.BeginCountingWorkers.Wait()
	master.Unlock()

	master.Logger.Printf("[Master.forwardRegistrations]: start forwardRegistrations... ")

loop:
	for {
		select {
		case <-master.Ctx.Done():

			master.Logger.Println("[Master.forwardRegistrations]: Thread-2 forwardRegistrations: exit")
			break loop

		case tmpWorker := <-master.tmpWorkers:

			// 1. decode tmpWorker
			workerInfo := entity.DecodeWorkerInfo(tmpWorker)
			workerInfo.PartyIndex = master.FLNetworkConfig.PartyIdToIndex[workerInfo.PartyID]

			// 2. check if this work already exist
			if master.FLNetworkConfig.RequiredWorkers[workerInfo.PartyID][workerInfo.WorkerID] == false {
				master.Lock()

				master.workers = append(master.workers, workerInfo)

				master.Unlock()
				master.beginCountDown.Broadcast()
				master.FLNetworkConfig.RequiredWorkers[workerInfo.PartyID][workerInfo.WorkerID] = true

			} else {
				master.Logger.Printf("[Master]: the worker %s already registered, skip \n", tmpWorker)
			}

			// 3. check if all worker registered
			master.Lock()
			if len(master.workers) == master.WorkerNum {
				// it is not strictly necessary for you to hold the lock on M sync.Mutex when calling C.Broadcast()
				master.allWorkerReady.Broadcast()
			}
			master.Unlock()
		}
	}
}
