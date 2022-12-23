
#!/bin/bash

echo "----- p0w0 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-3-91-16-69.compute-1.amazonaws.com "docker images lemonwyc/falcon:latest")"
echo " "

echo "-----p1w0 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-54-164-50-149.compute-1.amazonaws.com "docker images lemonwyc/falcon:latest")"
echo " "

echo "----- p2w0 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-18-215-154-119.compute-1.amazonaws.com "docker images lemonwyc/falcon:latest")"
echo " "

echo "----- p0w1 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-3-87-245-116.compute-1.amazonaws.com "docker images lemonwyc/falcon:latest")"
echo " "

echo "----- p1w1 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-34-224-102-45.compute-1.amazonaws.com "docker images lemonwyc/falcon:latest")"
echo " "

echo "----- p2w1 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-3-94-163-69.compute-1.amazonaws.com "docker images lemonwyc/falcon:latest")"
echo " "

echo "----- p0w2 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-52-201-253-53.compute-1.amazonaws.com "docker images lemonwyc/falcon:latest")"
echo " "

echo "----- p1w2 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-54-227-102-114.compute-1.amazonaws.com "docker images lemonwyc/falcon:latest")"
echo " "

echo "----- p2w2 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-34-230-88-196.compute-1.amazonaws.com "docker images lemonwyc/falcon:latest")"
echo " "

echo "----- p0w3 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-34-229-151-146.compute-1.amazonaws.com "docker images lemonwyc/falcon:latest")"
echo " "

echo "----- p1w3 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-54-242-48-254.compute-1.amazonaws.com "docker images lemonwyc/falcon:latest")"
echo " "

echo "----- p2w3 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-34-201-146-104.compute-1.amazonaws.com "docker images lemonwyc/falcon:latest")"
echo " "
