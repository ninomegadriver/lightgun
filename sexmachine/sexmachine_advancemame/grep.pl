#!/usr/bin/perl

open(F, "./advance/linux/vfb.c");
while(!eof(F)){
  $l = <F>;
  if(($l =~ /\)\n/) and ($l =~ /^[A-z]/)){
    print $l;
  }
}
