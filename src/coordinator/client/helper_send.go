package client

import (
	"bytes"
	"coordinator/logger"
	"encoding/json"
	"io/ioutil"
	"net/http"
	"strings"
	"time"
)

func Get(addr string) string {

	logger.Do.Println("send get requests to ", addr)

	resp, err := http.Get(addr)

	if err != nil {
		logger.Do.Println(err)
		panic(err)
	}

	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)

	if err != nil {
		logger.Do.Println(err)
		panic(err)
	}

	if resp.StatusCode > 200 && resp.StatusCode <= 299{
		logger.Do.Println("Get request Error ", resp.StatusCode, string(body))
		panic(err)
	}

	logger.Do.Println("Get response is ",string(body))
	return string(body)
}

func PostForm(addr string, data map[string][]string) error {
	/*

		eg:
			addr: "http://localhost:9089/submit"
			data：= url.Values {
					"name": {"John Doe"},
					"occupation": {"gardener"}}

	*/
	addr = "http://" + strings.TrimSpace(addr)

	logger.Do.Printf("Sending post request to address: %q", addr)

	NTimes := 20

	var err error
	var resp *http.Response

	for {
		if NTimes<0{
			return err
		}

		resp, err = http.PostForm(addr, data)

		if err != nil{
			logger.Do.Println("Post Requests Error happens retry ..... ,", err)
			time.Sleep(time.Second*4)
			NTimes--
		}else{
			break
		}
	}

	var res map[string]interface{}

	_ = json.NewDecoder(resp.Body).Decode(&res)

	logger.Do.Println("PostForm, res: ", res)

	return nil
}

func PostJson(addr string, js string) {
	/*
		eg:
			addr: "http://localhost:9089/submit"
			var js string =`{"Name":"naili","Age":123}`
	*/
	addr = strings.TrimSpace(addr)
	var jsonStr = []byte(js)
	req, err := http.NewRequest(http.MethodPost, addr, bytes.NewBuffer(jsonStr))
	if err != nil {
		panic("connection errro")
	}
	req.Header.Set("Content-Type", "application/json")

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		panic(err)
	}
	defer resp.Body.Close()

	logger.Do.Println("response Status:", resp.Status)
	logger.Do.Println("response Headers:", resp.Header)
	body, _ := ioutil.ReadAll(resp.Body)
	logger.Do.Println("response Body:", string(body))
}
