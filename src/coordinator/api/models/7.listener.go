package models

func (ms *MetaStore) ListenerAdd(ListenerAddr string) (error, *Listeners) {
	u := &Listeners{
		ListenerAddr: ListenerAddr,
	}

	err := ms.Db.Create(u).Error
	return err, u
}

func (ms *MetaStore) ListenerDelete(ListenerAddr string) error {

	e := ms.Db.Where("listener_add = ?", ListenerAddr).Delete(Listeners{}).Error

	return e
}
