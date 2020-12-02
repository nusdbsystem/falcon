CREATE USER 'falcon'@'localhost' IDENTIFIED BY 'falcon';
SET GLOBAL max_connections = 500;
CREATE DATABASE falcon;
GRANT ALL PRIVILEGES ON falcon.* TO 'falcon'@'localhost';
