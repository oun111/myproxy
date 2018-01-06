## What is MYPROXY

 - A proxy application that provides: 
    1. management on backend MYSQL databse tables 
    2. shardings on these tables
 - User may access backend tables through myproxy by normal MYSQL client tools 


## Features
 - supports native MYSQL server/client communication protocols v4.1
 - supports 10k+ clients concurrency 
 - supports dynamic scalability of MYSQL databases
 - supports the commonly used SQLs
 - supports SQL proxy functionalities, for example, ORACLE SQL -> MYSQL SQL
 - supports 2 sharding modes: horizontal and vertical


## Structure
 ![Alt text](https://github.com/oun111/images/blob/master/myproxy_structure.png)

Here the diagram consist of 3 parts:
 - MYSQL clients: sending requests and fetching datas from MYPROXY
 - MYPROXY: 
    1. accepting and processing client requests
    2. forwarding and routing them to backend MYSQL servers
    3. fetching responses from backend and forwarding them to clients
 - MYSQL servers: storing real datas and processing requests from MYPROXY
    
## Process Flow


## Dynamic Scalability


## SQL Proxy


## Shardings
 - 

## HOWTO

 * start up MYPROXY
 * access it just as you are accessing a real MYSQL server like this:
 
 ![Alt text](https://github.com/oun111/images/blob/master/myproxy_screen.jpg)
