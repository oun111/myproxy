

#drop database if exists test_db ;
#create database test_db ;
#
#use test_db ;
#grant all on test_db to mysql ;

drop table if exists test_tbl;
create table test_tbl 
(
  id INT(10) AUTO_INCREMENT NOT NULL PRIMARY KEY COMMENT 'product id',
	name char(30) NOT NULL COMMENT 'product name',
	price float(10) NOT NULL COMMENT 'product price',
	size int(10) NOT NULL COMMENT 'product size'
) CHARSET=utf8;

insert into test_tbl (name,price,size) values('orage',2.334,12);
insert into test_tbl (name,price,size) values('orage1',1.4,4);
insert into test_tbl (name,price,size) values('orage2',1.9,5);
insert into test_tbl (name,price,size) values('orage3',4.55,16);
insert into test_tbl (name,price,size) values('board',25.01,50);
insert into test_tbl (name,price,size) values('banana',2.17,3);
insert into test_tbl (name,price,size) values('大蕉1',3.557,10);
insert into test_tbl (name,price,size) values('umbella',2.12346,7);

drop table if exists test_tbl_1;
create table test_tbl_1 
(
  id INT(10) AUTO_INCREMENT NOT NULL PRIMARY KEY COMMENT 'product id',
  name MEDIUMTEXT NOT NULL COMMENT 'product name',
	price float(10) NOT NULL COMMENT 'product price',
	size int(10) NOT NULL COMMENT 'product size'
) CHARSET=utf8;

insert into test_tbl_1 (name,price,size) values(lpad('12345',4096,'12345'),2.334,12);
insert into test_tbl_1 (name,price,size) values(lpad('12345',4096,'12345'),2.1,5);
insert into test_tbl_1 (name,price,size) values(lpad('12345',4096,'12345'),334,1);
insert into test_tbl_1 (name,price,size) values(lpad('12345',4096,'12345'),33.4,10);
insert into test_tbl_1 (name,price,size) values(lpad('12345',4096,'12345'),3.34,7);


drop table if exists test_tbl_2;
create table test_tbl_2
(
  id INT(10) AUTO_INCREMENT NOT NULL PRIMARY KEY COMMENT 'product id',
	name char(30) NOT NULL COMMENT 'product name',
	price float(10) NOT NULL COMMENT 'product price',
	size int(10) NOT NULL COMMENT 'product size'
) CHARSET=utf8;

insert into test_tbl_2 (name,price,size) values('apple2',5.034,2);
insert into test_tbl_2 (name,price,size) values('watermelon3',0.547,10);
insert into test_tbl_2 (name,price,size) values('banana',4.67,8);
insert into test_tbl_2 (name,price,size) values('peach2',4,5);
