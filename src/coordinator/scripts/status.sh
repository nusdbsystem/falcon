
title()
{
    title="| $1 |"
    edge=$(echo "$title" | sed 's/./-/g')
    echo "$edge"
    echo "$title"
    echo "$edge"
}



if [ ! -n "$1" ] ;then
      # start all services
    title "user pods info"
    kubectl get pods -o wide
    title "user svc info"
    kubectl get svc -o wide
    title "user deployments info"
    kubectl get deployments -o wide
    title "user nodes info"
    kubectl get nodes -o wide
    title "user config map"
    kubectl get configmap -o wide

    title "user pv"
    kubectl get pv -o wide

    title "user pvc"
    kubectl get pvc -o wide

    title "system pods info"
    kubectl get pods -o wide --namespace=kube-system
    title "system svc info"
    kubectl get svc -o wide --namespace=kube-system
    title "system deployments info"
    kubectl get deployments -o wide --namespace=kube-system

    title "ingress deployments info"
    kubectl get deployments -o wide --namespace=ingress-nginx
    title "ingress svc  info"
    kubectl get svc -o wide --namespace=ingress-nginx
    title "ingress pods info"
    kubectl get pods -o wide --namespace=ingress-nginx
    title "system helm list"
    helm list

elif [[ $1 = "user" ]];then
      title "user pods info"
      kubectl get pods -o wide
      title "user svc info"
      kubectl get svc -o wide
      title "user deployments info"
      kubectl get deployments -o wide
      title "user nodes info"
      kubectl get nodes -o wide
      title "user config map"
      kubectl get configmap

      title "user pv"
      kubectl get pv -o wide

      title "user pvc"
      kubectl get pvc -o wide

elif [[ $1 = "system" ]];then
      title "system pods info"
      kubectl get pods -o wide --namespace=kube-system
      title "system svc info"
      kubectl get svc -o wide --namespace=kube-system
      title "system deployments info"
      kubectl get deployments -o wide --namespace=kube-system

      title "ingress deployments info"
      kubectl get deployments -o wide --namespace=ingress-nginx
      title "ingress svc  info"
      kubectl get svc -o wide --namespace=ingress-nginx
      title "ingress pods info"
      kubectl get pods -o wide --namespace=ingress-nginx
      title "system helm list"
      helm list
else
    title "Un-support arguments, please see the help doc"
fi



