package test

import (
	"coordinator/distributed/entitiy"
	crand "crypto/rand"
	"encoding/base64"
	"fmt"
	"testing"
)

func TestRandomFunc(t *testing.T) {

	a := entitiy.ShutdownReply{Ntasks: 123}

	res := entitiy.EncodeDoTaskArgsGeneral(&a)
	fmt.Println(res)

	rr := entitiy.ShutdownReply{}
	entitiy.DecodeDoTaskArgsGeneral(res, &rr)
	fmt.Println(rr.Ntasks)

}

func TestRandomFunc2(t *testing.T) {
	a := func(n int) string {
		b := make([]byte, 2*n)
		crand.Read(b)
		s := base64.URLEncoding.EncodeToString(b)
		return s[0:n]
	}

	fmt.Println(a(20))
}
