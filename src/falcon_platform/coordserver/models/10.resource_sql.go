package models

import "github.com/jinzhu/gorm"

func (jobDB *JobDB) GetPorts() []uint {
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
	//  this func only called internally, the process logic is the same
	//  so define error hand logic here for convience
	if err!=nil{
		panic(err)
	}
	return res
}

func (jobDB *JobDB) AddPort(tx *gorm.DB,port uint) (error, *PortRecord) {
	u := &PortRecord{
		Port: port,
		IsDelete: 0,
	}

	err := tx.Create(u).Error

	return err, u
}


func (jobDB *JobDB) DeletePort(tx *gorm.DB ,port uint) (error, *PortRecord) {
	u := &PortRecord{}

	err := tx.Model(u).
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