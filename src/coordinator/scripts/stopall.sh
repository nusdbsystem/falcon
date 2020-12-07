. ./deploy/property/svc.properties
. userdefined.properties


# load env variables
if [ $DATA_BASE_PATH ];then
	echo "DATA_BASE_PATH is exist, and echo to = $DATA_BASE_PATH"
else
	export DATA_BASE_PATH=$PWD
fi


kubectl delete all --all
kubectl delete deployment,svc mysql
kubectl delete pvc mysql-pv-claim
kubectl delete pv mysql-pv-volume
kubectl delete pvc $MASTER_STORAGE-pvc
kubectl delete pv $MASTER_STORAGE-pv
kubectl delete pvc $LISTENER_STORAGE-pvc
kubectl delete pv $LISTENER_STORAGE-pv

kubectl delete configmap mysql-initdb-config
kubectl delete configmap coord-config
kubectl delete configmap redis-config
kubectl delete configmap redis-envs

rm -rf $DATA_BASE_PATH/database
#rm -rf $DATA_BASE_PATH/logs
