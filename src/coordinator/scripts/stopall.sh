. ./deploy/property/svc.properties
kubectl delete all --all
kubectl delete deployment,svc mysql
kubectl delete pvc mysql-pv-claim
kubectl delete pv mysql-pv-volume
kubectl delete configmap mysql-initdb-config
kubectl delete configmap coord-config
kubectl delete pvc $MASTER_STORAGE-pvc
kubectl delete pv $MASTER_STORAGE-pv
kubectl delete pvc $LISTENER_STORAGE-pvc
kubectl delete pv $LISTENER_STORAGE-pv
kubectl delete configmap redis-config
