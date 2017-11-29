#!/usr/bin/perl -w

sub read_elapsed
{
  my $strl= $_[0];
  my $a_ref = $_[1];
  my $sum_ref = $_[2];
  my $FD ;

  open $FD,"<",$strl or 
    die "can not open $strl";

  while (<$FD>) {
    if (/elapsed: (\d+)/) {
      push $a_ref,$1;
      $$sum_ref += $1 ;
    }
  }
  close $FD;
}

my @ary_r;
my @ary_nr ;
my $sum_r = 0;
my $sum_nr = 0;

&read_elapsed($ARGV[0],\@ary_r,\$sum_r);
&read_elapsed($ARGV[1],\@ary_nr,\$sum_nr);

print "  r\t\tnr\n";
foreach(0..$#ary_r) {
  print "$_: ";
  print "$ary_r[$_]\t$ary_nr[$_]\t\t".
    ($ary_r[$_]>$ary_nr[$_]?"bigger ":"smaller ").
    "\n";
}
print "avg:\t".($sum_r/$#ary_r/1000).
  "(ms)\t".($sum_nr/$#ary_nr/1000)."(ms)\n";

