package models



func (ms *MetaStore) GetPorts() (error, []uint){
	/**
	 * @Author
	 * @Description fetch all records
	 * @Date 4:43 下午 6/12/20
	 * @Param
	 * @return
	 **/
	var u []*PortRecord
	var res []uint
	err := ms.Db.Find(&u).Error

	if err==nil{
		for _, item := range u{
			res = append(res, item.Port)
		}
	}

	return err, res
}

func (ms *MetaStore) AddPort(port uint) (error, *PortRecord) {
	u := &PortRecord{
		Port: port,
		IsDelete: 0,
	}

	err := ms.Db.Create(u).Error

	return err, u
}


func (ms *MetaStore) DeletePort(port uint) (error, *PortRecord) {
	u := &PortRecord{}

	err := ms.Db.Model(u).
		Where("port = ?", port).
		Update("isdelete", 1).Error
	return err, u
}

func (ms *MetaStore) CheckPort(port uint) bool {

	u := &PortRecord{}
	err := ms.Db.First(u, "port = ?", port ).Error
	if err == nil{
		if u.Port==0{
			return false
		}else{
			return true
		}
	}else{
		return false
	}
}