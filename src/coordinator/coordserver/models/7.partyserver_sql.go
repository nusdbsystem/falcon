package models

func (jobDB *JobDB) PartyServerAdd(PartyServerAddr, Port string) (error, *PartyServer) {
	u := &PartyServer{
		PartyServerAddr: PartyServerAddr,
		Port: Port,
	}

	err := jobDB.Db.Create(u).Error
	return err, u
}

func (jobDB *JobDB) PartyServerDelete(PartyServerAddr string) error {

	e := jobDB.Db.Where("partyserver_addr = ?", PartyServerAddr).Delete(PartyServer{}).Error

	return e
}


func (jobDB *JobDB) PartyServerGet(PartyServerAddr string) (error, *PartyServer) {

	u := &PartyServer{}
	err := jobDB.Db.Where("partyserver_addr = ?", PartyServerAddr).First(u).Error
	return err, u
}
