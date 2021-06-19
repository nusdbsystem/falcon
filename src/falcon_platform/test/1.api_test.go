package test

import (
	"encoding/json"
	"falcon_platform/common"
	"falcon_platform/coordserver/models"
	"falcon_platform/logger"
	"fmt"
	"io/ioutil"
	"os"
	"testing"
)

func TestSql(t *testing.T) {

	jobDB := models.InitJobDB()

	jobDB.Connect()

	e, u := jobDB.JobGetByJobID(1)
	if e != nil {
		panic(e)
	}

	logger.Log.Println(u)

}

func TestJson(t *testing.T) {
	type Bird struct {
		Species     string
		Description string
	}

	birdJson := `{"species": "pigeon", 
				  "description": "likes to perch on rocks"}`
	var bird Bird

	_ = json.Unmarshal([]byte(birdJson), &bird)
	logger.Log.Printf(
		"Species: %s, Description: %s",
		bird.Species, bird.Description)
}

func TestParseJson(t *testing.T) {
	jsonFile, err := os.Open("/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/train_jobs/job.json")
	logger.Log.Println(err)

	byteValue, _ := ioutil.ReadAll(jsonFile)

	// we initialize our Users array
	var job common.TrainJob

	e2 := json.Unmarshal(byteValue, &job)
	logger.Log.Println(e2)

	b, err := json.Marshal(job.PartyInfoList)
	logger.Log.Println(string(b), err)

	jb, _ := json.Marshal(job.Tasks.ModelTraining.InputConfigs.AlgorithmConfig)
	res := common.LogisticRegression{}
	_ = json.Unmarshal(jb, &res)
	fmt.Println(res)

}
