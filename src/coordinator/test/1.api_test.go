package test

import (
	"coordinator/api/models"
	"coordinator/common"
	"coordinator/logger"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"testing"
)

func TestSql(t *testing.T) {

	ms := models.InitMetaStore()

	ms.Connect()
	e, u := ms.JobGetByJobID(1)
	ms.Commit(e)
	ms.DisConnect()

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
	jsonFile, err := os.Open("/Users/nailixing/GOProj/src/github.com/falcon/src/coordinator/data/dsl.json")
	logger.Do.Println(err)

	byteValue, _ := ioutil.ReadAll(jsonFile)

	// we initialize our Users array
	var dsl common.DSL

	e2 := json.Unmarshal(byteValue, &dsl)
	logger.Do.Println(e2)

	b, err := json.Marshal(dsl.PartyInfos)
	logger.Do.Println(string(b), err)

	jb, _ := json.Marshal(dsl.Tasks.ModelTraining.InputConfigs.AlgorithmConfig)
	res := common.LogisticRegression{}
	_=json.Unmarshal(jb,&res)
	fmt.Println(res)

}
