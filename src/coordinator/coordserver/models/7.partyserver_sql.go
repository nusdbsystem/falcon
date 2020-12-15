package models

func (ms *MetaStore) PartyServerAdd(PartyServerAddr, Port string) (error, *PartyServers) {
	u := &PartyServers{
		PartyServerAddr: PartyServerAddr,
		Port: Port,
	}

	err := ms.Db.Create(u).Error
	return err, u
}

func (ms *MetaStore) PartyServerDelete(PartyServerAddr string) error {

	e := ms.Db.Where("partyserver_addr = ?", PartyServerAddr).Delete(PartyServers{}).Error

	return e
}


func (ms *MetaStore) PartyServerGet(PartyServerAddr string) (error, *PartyServers) {

	u := &PartyServers{}
	err := ms.Db.Where("partyserver_addr = ?", PartyServerAddr).First(u).Error
	return err, u
}
