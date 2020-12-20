package test

import (
	"coordinator/coordserver/models"
	"coordinator/common"
	"coordinator/logger"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"testing"
)

func TestSql(t *testing.T) {

	jobDB := models.InitJobDB()

	jobDB.Connect()

	e, u := jobDB.JobGetByJobID(1)
	if e!=nil{
		panic(e)
	}

	jobDB.Disconnect()

	logger.Do.Println(u)

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
	logger.Do.Printf(
		"Species: %s, Description: %s",
		bird.Species, bird.Description)
}

func TestParseJson(t *testing.T) {
	jsonFile, err := os.Open("/Users/nailixing/GOProj/src/github.com/falcon/src/coordinator/data/job.json")
	logger.Do.Println(err)

	byteValue, _ := ioutil.ReadAll(jsonFile)

	// we initialize our Users array
	var job common.TrainJob

	e2 := json.Unmarshal(byteValue, &job)
	logger.Do.Println(e2)

	b, err := json.Marshal(job.PartyInfo)
	logger.Do.Println(string(b), err)

	jb, _ := json.Marshal(job.Tasks.ModelTraining.InputConfigs.AlgorithmConfig)
	res := common.LogisticRegression{}
	_=json.Unmarshal(jb,&res)
	fmt.Println(res)

}
