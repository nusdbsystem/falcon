package middleware

import (
	"coordinator/coordserver/entity"
	"net/http"
)

// middle ware used to verify the methods
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

		// disConnect to db
		context.JobDB.DisConnect()
	}
}

func UserVerify(w http.ResponseWriter, r *http.Request, ctx *entity.Context) error {

	//todo check user id, got id from token

	//token := r.Header.Get("token")

	e, u := ctx.JobDB.GetUserByUserID(1)

	if e != nil {
		http.Error(w, "User not exist", http.StatusBadRequest)
		return e
	}

	ctx.UsrId = u.UserID

	return nil
}
