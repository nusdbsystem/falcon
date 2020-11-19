package client

import (
	"bytes"
	"encoding/json"
	"io/ioutil"
	"log"
	"net/http"
	"strings"
)

func Get(addr string) error {
	addr = strings.TrimSpace(addr)
	resp, err := http.Get(addr)

	if err != nil {
		panic(err)
	}

	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)

	if err != nil {

		panic(err)
	}

	log.Println(string(body))
	return nil
}

func PostForm(addr string, data map[string][]string) error {
	/*

		eg:
			addr: "http://localhost:9089/submit"
			data：= url.Values {
					"name": {"John Doe"},
					"occupation": {"gardener"}}

	*/
	log.Printf("%q", addr)
	addr = "http://" + strings.TrimSpace(addr)
	resp, err := http.PostForm(addr, data)
	if err != nil {
		log.Println("PostForm Error happens,", err)
		//panic(err)
		return err
	}
	var res map[string]interface{}

	_ = json.NewDecoder(resp.Body).Decode(&res)

	log.Println("PostForm, res: ", res)

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

	log.Println("response Status:", resp.Status)
	log.Println("response Headers:", resp.Header)
	body, _ := ioutil.ReadAll(resp.Body)
	log.Println("response Body:", string(body))
}
