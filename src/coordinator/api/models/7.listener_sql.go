package models

func (ms *MetaStore) ListenerAdd(ListenerAddr, Port string) (error, *Listeners) {
	u := &Listeners{
		ListenerAddr: ListenerAddr,
		Port: Port,
	}

	err := ms.Db.Create(u).Error
	return err, u
}

func (ms *MetaStore) ListenerDelete(ListenerAddr string) error {

	e := ms.Db.Where("listener_addr = ?", ListenerAddr).Delete(Listeners{}).Error

	return e
}


func (ms *MetaStore) ListenerGet(ListenerAddr string) (error, *Listeners) {

	u := &Listeners{}
	err := ms.Db.Where("listener_addr = ?", ListenerAddr).First(u).Error
	return err, u
}
