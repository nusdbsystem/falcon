
#!/bin/bash

echo "----- p0w0 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-54-234-220-224.compute-1.amazonaws.com "docker images lemonwyc/falcon:vldb23")"
echo " "

echo "-----p1w0 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-18-234-111-41.compute-1.amazonaws.com "docker images lemonwyc/falcon:vldb23")"
echo " "

echo "----- p2w0 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-54-157-11-53.compute-1.amazonaws.com "docker images lemonwyc/falcon:vldb23")"
echo " "

echo "----- p0w1 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-54-86-26-168.compute-1.amazonaws.com "docker images lemonwyc/falcon:vldb23")"
echo " "

echo "----- p1w1 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-18-212-215-93.compute-1.amazonaws.com "docker images lemonwyc/falcon:vldb23")"
echo " "

echo "----- p2w1 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-3-90-12-224.compute-1.amazonaws.com "docker images lemonwyc/falcon:vldb23")"
echo " "

echo "----- p0w2 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-54-226-155-92.compute-1.amazonaws.com "docker images lemonwyc/falcon:vldb23")"
echo " "

echo "----- p1w2 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-50-19-175-106.compute-1.amazonaws.com "docker images lemonwyc/falcon:vldb23")"
echo " "

echo "----- p2w2 -----"
echo "$(ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" ubuntu@ec2-54-198-246-148.compute-1.amazonaws.com "docker images lemonwyc/falcon:vldb23")"
echo " "
