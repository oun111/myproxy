#!/usr/bin/perl -w

use strict;

die "needs 1 argument" unless $#ARGV==0;

if ($ARGV[$#ARGV]==0 || $ARGV[$#ARGV]==1) {
  system "rm -f /tmp/myproxy*";
  system "rm -f ./__db*";
  #system "rm -f core";
  system "killall myproxy_app" ;
  if ($ARGV[$#ARGV]==1) {
#    system "./myproxy_app  >/dev/null 2>&1 &  " ;
    system "./src/myproxy_app -c ./conf/configure_dummy.json &  " ;
  }
}

