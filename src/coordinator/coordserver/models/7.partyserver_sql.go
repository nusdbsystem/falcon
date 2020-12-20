package models

import "github.com/jinzhu/gorm"

func (jobDB *JobDB) PartyServerAdd(tx *gorm.DB,PartyServerAddr, Port string) (error, *PartyServer) {
	u := &PartyServer{
		PartyServerAddr: PartyServerAddr,
		Port: Port,
	}

	err := tx.Create(u).Error
	return err, u
}

func (jobDB *JobDB) PartyServerDelete(tx *gorm.DB,PartyServerAddr string) error {

	e := tx.Where("partyserver_addr = ?", PartyServerAddr).Delete(PartyServer{}).Error

	return e
}


func (jobDB *JobDB) PartyServerGet(PartyServerAddr string) (error, *PartyServer) {

	u := &PartyServer{}
	err := jobDB.Db.Where("partyserver_addr = ?", PartyServerAddr).First(u).Error
	return err, u
}
