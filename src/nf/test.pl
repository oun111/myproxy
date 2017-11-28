#!/usr/bin/perl -w

use strict;

die "needs 1 argument" unless $#ARGV==0;

if ($ARGV[$#ARGV]==0 || $ARGV[$#ARGV]==1) {
  system "killall test_app";
  system "rm -rf /tmp/fwork*";
  if ($ARGV[$#ARGV]==1) {
    system "./test_app &";
  }
}

