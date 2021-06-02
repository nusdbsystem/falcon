package models

import "gorm.io/gorm"

func (jobDB *JobDB) GetUserByUserID(userId uint) (error, *User) {
	u := &User{}

	err := jobDB.DB.First(u, userId).Error

	return err, u
}

func (jobDB *JobDB) CreateAdminUser(tx *gorm.DB) error {

	e, u := jobDB.GetUserByUserID(1)
	if u.UserID == 0 {
		e = tx.Create(&User{UserID: 1, Name: "admin"}).Error
		return e
	}
	return e

}
