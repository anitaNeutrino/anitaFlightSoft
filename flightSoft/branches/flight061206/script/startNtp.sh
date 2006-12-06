#/bin/bash
date > /tmp/didIDoThis
sudo /etc/init.d/ntpd restart > /tmp/yesIDid 2>&1