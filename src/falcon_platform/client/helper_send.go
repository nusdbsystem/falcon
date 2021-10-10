package client

import (
	"bytes"
	"falcon_platform/logger"
	"net/http"
	"strings"
	"time"
)

const nTimes int = 5   // retry times
const interval int = 1 // wait seconds

func Get(url string) (*http.Response, error) {

	curTime := nTimes

	var err error
	var resp *http.Response

	for {
		if curTime < 0 {
			return resp, err
		}

		resp, err = http.Get(url)

		if err != nil {
			logger.Log.Printf("Get Requests, Error: %s; retry after %d seconds\n", err, interval)
			time.Sleep(time.Second * time.Duration(interval))
			curTime--
		} else {
			break
		}
	}
	return resp, nil
}

func PostForm(addr string, data map[string][]string) (*http.Response, error) {
	/*

		eg:
			addr: "http://localhost:9089/submit"
			data：= addr.Values {
					"name": {"John Doe"},
					"occupation": {"gardener"}}

	*/
	url := "http://" + strings.TrimSpace(addr)

	curTime := nTimes

	var err error
	var resp *http.Response

	for {
		if curTime < 0 {
			return resp, err
		}

		resp, err = http.PostForm(url, data)

		if err != nil {
			logger.Log.Printf("Post Requests, Error: %s; retry after %d seconds\n", err, interval)
			time.Sleep(time.Second * time.Duration(interval))
			curTime--
		} else {
			break
		}
	}
	return resp, nil
}

func PostJson(url string, js string) (*http.Response, error) {
	/*
		eg:
			url: "http://localhost:9089/submit"

			map -> string
			values := map[string]string{"username": username, "password": password}
			js, _ := json.Marshal(values)

			struct -> string
			user := &User{Name: "Frank"}
			jsb, _ := json.Marshal(user)
			js = string(jsb)
	*/

	url = strings.TrimSpace(url)
	var jsonStr = []byte(js)

	curTime := nTimes

	var err error
	var resp *http.Response

	req, err := http.NewRequest(http.MethodPost, url, bytes.NewBuffer(jsonStr))
	if err != nil {
		logger.Log.Printf("Post Requests Error: %s \n", err)
	}
	req.Header.Set("Content-Type", "application/json")
	client := &http.Client{}

	for {
		if curTime < 0 {
			return resp, err
		}
		resp, err = client.Do(req)

		if err != nil {
			logger.Log.Printf("Post Requests Error: %s; retry after %d seconds\n", err, interval)
			time.Sleep(time.Second * time.Duration(interval))
			curTime--
		} else {

			break
		}
	}
	return resp, nil
}
