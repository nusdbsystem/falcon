package middleware

import (
	"falcon_platform/coordserver/entity"
	"falcon_platform/exceptions"
	"fmt"
	"net/http"
)

// middleware to initiate context
func InitContext(f func(w http.ResponseWriter, r *http.Request, c *entity.Context),
	SysLvPath []string) http.HandlerFunc {

	return func(w http.ResponseWriter, r *http.Request) {

		context := entity.InitContext()

		// connect to db
		context.JobDB.Connect()

		// check if needs verify
		var isVerify = true

		for _, v := range SysLvPath {
			if r.URL.Path == v {
				isVerify = false
			}
		}

		// verify user exist or not

		if isVerify {
			e := UserVerify(w, r, context)
			if e != nil {
				return
			}
		}

		f(w, r, context)
	}
}

func UserVerify(w http.ResponseWriter, r *http.Request, ctx *entity.Context) error {

	//todo check user id, got id from token

	//token := r.Header.Get("token")
	userId := 1
	userName := "admin"
	e, u := ctx.JobDB.GetUserByUserID(uint(userId))

	if e != nil {
		errMsg := fmt.Sprintf("User Id %d name %s not exist", userId, userName)
		exceptions.HandleHttpError(w, r, http.StatusBadRequest, errMsg)
		return e
	}

	ctx.UsrId = u.UserID

	return nil
}
