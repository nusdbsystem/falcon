package models

func (ms *MetaStore) GetUserByUserID(userId uint) (error, *User) {
	u := &User{}

	err := ms.Db.First(u, userId).Error

	return err, u
}

func (ms *MetaStore) CreateAdminUser() error {

	e, u := ms.GetUserByUserID(1)
	if u.UserID == 0 {
		e := ms.Db.Create(&User{UserID: 1, Name: "admin"}).Error
		return e
	}
	return e

}
