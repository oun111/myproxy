
USE_UNQLITE  := 0
USE_BDB      := 0
USE_SQLITE   := 0
USE_LEVELDB  := 1
USE_MULTIMAP := 1

#
# bdb
#

WORKPLACE     := /home/user1/work
#WORKPLACE     := /data/zyw/work

ifeq ($(USE_BDB),1)
  BDB_BASE      := $(WORKPLACE)/db-6.2.23/install

  BDB_INCLUDE   := $(BDB_BASE)/include
  BDB_LIB       := $(BDB_BASE)/lib

  BDB_CFLAGS    := -I$(BDB_INCLUDE)  -DUSE_BDB=1 
  BDB_LDFLAGS   := -L$(BDB_LIB) -ldb
endif

#
# unqlite
#

ifeq ($(USE_UNQLITE),1)
  UNQLITE_BASE    :=$(WORKPLACE)/unqlite

  UNQLITE_INCLUDE := $(UNQLITE_BASE)
  UNQLITE_LIB     := $(UNQLITE_BASE)

  UNQLITE_CFLAGS  := -I$(UNQLITE_INCLUDE) -DUSE_UNQLITE=1
  UNQLITE_LDFLAGS := -L$(UNQLITE_LIB) 
endif 


#
# sqlite
#

ifeq ($(USE_SQLITE),1)
  SQLITE_BASE    :=$(WORKPLACE)/sqlite-src-3170000/

  SQLITE_INCLUDE := -I$(SQLITE_BASE)/include -I$(SQLITE_BASE)/src \
                    -I$(SQLITE_BASE)
  SQLITE_LIB     := $(SQLITE_BASE)/install/lib

  SQLITE_CFLAGS  := $(SQLITE_INCLUDE) -DUSE_SQLITE=1
  SQLITE_LDFLAGS := -L$(SQLITE_LIB) -lsqlite3
endif 

#
# leveldb
#

ifeq ($(USE_LEVELDB),1)
  LEVELDB_BASE    :=$(WORKPLACE)/leveldb-master

  LEVELDB_INCLUDE := -I$(LEVELDB_BASE)/include/
  LEVELDB_LIB     := $(LEVELDB_BASE)/out-static

  LEVELDB_CFLAGS  := $(LEVELDB_INCLUDE) -DUSE_LEVELDB=1
  LEVELDB_LDFLAGS := -L$(LEVELDB_LIB) -lleveldb -pthread
endif 

#
# stl-multimap
#

ifeq ($(USE_MULTIMAP),1)
  MULTIMAP_CFLAGS  := -DUSE_MULTIMAP=1
endif 



DBWPR_BASE      := $(WORKPLACE)/dbwrapper
DBWPR_CFLAGS    := $(UNQLITE_CFLAGS) $(BDB_CFLAGS) $(SQLITE_CFLAGS) $(LEVELDB_CFLAGS) $(MULTIMAP_CFLAGS) -I$(DBWPR_BASE)
DBWPR_LDFLAGS   := $(UNQLITE_LDFLAGS) $(BDB_LDFLAGS) $(SQLITE_LDFLAGS) $(LEVELDB_LDFLAGS) -L$(DBWPR_BASE) 

