CREATE USER 'falcon'@'%' IDENTIFIED BY 'falcon';
GRANT ALL PRIVILEGES ON *.* to 'falcon'@'%' identified by 'falcon';
FLUSH PRIVILEGES;
CREATE DATABASE falcon;
SET GLOBAL max_connections = 500;