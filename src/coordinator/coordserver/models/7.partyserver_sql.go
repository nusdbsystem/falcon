package models

func (jobDB *JobDB) PartyServerAdd(PartyServerUrl, Port string) (error, *PartyServer) {
	u := &PartyServer{
		PartyServerUrl: PartyServerUrl,
		Port: Port,
	}

	err := jobDB.Db.Create(u).Error
	return err, u
}

func (jobDB *JobDB) PartyServerDelete(PartyServerUrl string) error {

	e := jobDB.Db.Where("partyserver_url = ?", PartyServerUrl).Delete(PartyServer{}).Error

	return e
}


func (jobDB *JobDB) PartyServerGet(PartyServerUrl string) (error, *PartyServer) {

	u := &PartyServer{}
	err := jobDB.Db.Where("partyserver_url = ?", PartyServerUrl).First(u).Error
	return err, u
}
