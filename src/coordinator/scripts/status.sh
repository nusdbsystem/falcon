
title()
{
    title="| $1 |"
    edge=$(echo "$title" | sed 's/./-/g')
    echo "$edge"
    echo "$title"
    echo "$edge"
}

title "pods info"
kubectl get pods -o wide
title "svc info"
kubectl get svc -o wide
title "deployments info"
kubectl get deployments -o wide
title "nodes info"
kubectl get nodes -o wide
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
title "helm list"
helm list


title "config map"
kubectl get configmap
