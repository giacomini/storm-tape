# Configuration Filebeat to Kafka
## Download Filebeat

curl -L -O https://artifacts.elastic.co/downloads/beats/filebeat/filebeat-8.8.2-linux-x86_64.tar.gz

##  Extract a gz file
tar xzvf filebeat-8.8.2-linux-x86_64.tar.gz

### Download key
wget -O https://apt.releases.hashicorp.com/gpg | sudo gpg --dearmor -o /usr/share/keyrings/hashicorp-archive-keyring.gpg

### For add to repository install utils and create file repo
yum install -y yum-utils

yum-config-manager --add-repo https://rpm.releases.hashicorp.com/RHEL/hashicorp.repo

sudo rpm --import https://packages.elastic.co/GPG-KEY-elasticsearch

vi /etc/yum.repos.d/elastic.repo
[elastic-8.x]
name=Elastic repository for 8.x packages
baseurl=https://artifacts.elastic.co/packages/8.x/yum
gpgcheck=1
gpgkey=https://artifacts.elastic.co/GPG-KEY-elasticsearch
enabled=1
autorefresh=1
type=rpm-md

### Install Filebeat
yum install filebeat

## Usage

### Input variables in file /etc/filebeat/modules.d/system.yml 
```
- module: system
# Syslog
  syslog:
    enabled: true
```

#Run modules:  

### On modules system in Filebeat
filebeat modules enable system

### On modules kafka in Filebeat
filebeat modules enable kafka

### Less instalated modules
filebeat modules list
 
## To configure
```
sudo filebeat -e -c /etc/filebeat/filebeat.yml


```
### Enable daemon filebeat
systemctl enable filebeat

### Start daemon Filebeat
systemctl start filebeat
 
### Status daemon 
systemctl status filebeat 

# For testing configuration file
filebeat test config -v

# To delete
yum remove filebeat



