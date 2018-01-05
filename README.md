## What is Myproxy
 - A proxy application provides: 
    1. management on backend MYSQL databse tables 
    2. shardings on these tables
 - User may access them through myproxy by normal MYSQL client applications 


## Features
 - supports native MYSQL server/client communication protocols v4.1
 - supports 10k+ clients concurrency 
 - supports dynamic scalability of MYSQL databases
 - supports the commonly used SQLs
 - supports SQL proxy functionalities, for example, ORACLE SQL -> MYSQL SQL
 - supports 2 sharding modes: horizontal and vertical


## Structure
 ![Alt text](https://github.com/oun111/images/blob/master/myproxy_structure.png)

When clients access myproxy, it will do the following things:
 - test if the incoming request needs to access the `real` datas
    - if it needs, request will be processed and forward to the backend MYSQL servers
    - if not, myproxy will process it itself and reply to client as soon as possible
 - waits for `data incoming` events from backend 
 - fetching results from backend, re-processing them, merging them, and sending replies to clients
    
## Process Flow


## Dynamic Scalability


## SQL Proxy


## Shardings
 - 

## HOWTO

 * start up MYPROXY
 * access it just as you are accessing a real MYSQL server like this:
 
 ![Alt text](https://github.com/oun111/images/blob/master/myproxy_screen.jpg)
