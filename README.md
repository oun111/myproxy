# ZAS

ZAS is a sql adaptor library, with which, you may access `MYSQL` with `ORACLE-SQL-STYLE` like this:

```
stream1.open("select id.nextval(), id from table1");
```

## Feature
 * do sql language adaptions: oracle -> mysql
 * database access tool
 * sql syntax analyze and checking
 * cross platforms: linux, windows
 * cross languages: c++, java, python

## Install

 * link zas library to your c++ applications
 * load zas library into your java/python applications
 
## Roadmap
 * `src`: source code of zas library
 * `tests`: test cases of zas
 * `win`: project files of zas under windows
 * `wrapper`: wrapper library for java/python
 
## Examples

 * see `test_cases` files under `tests` directory for example
