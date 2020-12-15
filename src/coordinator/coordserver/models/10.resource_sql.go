package models



func (jobDB *JobDB) GetPorts() (error, []uint){
	/**
	 * @Author
	 * @Description fetch all records
	 * @Date 4:43 下午 6/12/20
	 * @Param
	 * @return
	 **/
	var u []*PortRecord
	var res []uint
	err := jobDB.Db.Find(&u).Error

	if err==nil{
		for _, item := range u{
			res = append(res, item.Port)
		}
	}

	return err, res
}

func (jobDB *JobDB) AddPort(port uint) (error, *PortRecord) {
	u := &PortRecord{
		Port: port,
		IsDelete: 0,
	}

	err := jobDB.Db.Create(u).Error

	return err, u
}


func (jobDB *JobDB) DeletePort(port uint) (error, *PortRecord) {
	u := &PortRecord{}

	err := jobDB.Db.Model(u).
		Where("port = ?", port).
		Update("isdelete", 1).Error
	return err, u
}

func (jobDB *JobDB) CheckPort(port uint) bool {

	u := &PortRecord{}
	err := jobDB.Db.First(u, "port = ?", port ).Error
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