package client

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io/ioutil"
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

	fmt.Println(string(body))
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
	fmt.Printf("%q", addr)
	addr = "http://" + strings.TrimSpace(addr)
	resp, err := http.PostForm(addr, data)
	if err != nil {
		fmt.Println("PostForm Error happens,", err)
		//panic(err)
		return err
	}
	var res map[string]interface{}

	_ = json.NewDecoder(resp.Body).Decode(&res)

	fmt.Println("PostForm, res: ", res)

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

	fmt.Println("response Status:", resp.Status)
	fmt.Println("response Headers:", resp.Header)
	body, _ := ioutil.ReadAll(resp.Body)
	fmt.Println("response Body:", string(body))
}
