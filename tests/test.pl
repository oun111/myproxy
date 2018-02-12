#!/usr/bin/perl -w

use strict;

die "needs 1 argument" unless $#ARGV==0;

if ($ARGV[$#ARGV]==0 || $ARGV[$#ARGV]==1) {
  system "rm -f /tmp/myproxy*";
  system "rm -f ./__db*";
  #system "rm -f core";
  system "killall myproxy_app" ;
  if ($ARGV[$#ARGV]==1) {
     system "sudo echo \"20000\" > /proc/sys/net/core/somaxconn" ;
     system "sudo echo \"20000\" > /proc/sys/net/ipv4/tcp_max_syn_backlog" ;
     system "ulimit -n 20000" ;

     system "./src/myproxy_app -c ./conf/configure_pressure_test.json >/tmp/a1 &  " ;
#    system "./src/myproxy_app -c ./conf/configure.json &  " ;
  }
}

