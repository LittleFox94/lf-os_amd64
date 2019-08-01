#!/usr/bin/env perl

use utf8;
use strict;

sub PML4_INDEX  { (shift >> 39) & 0x1FF }
sub PDP_INDEX   { (shift >> 30) & 0x1FF }
sub PD_INDEX    { (shift >> 21) & 0x1FF }
sub PT_INDEX    { (shift >> 12) & 0x1FF }
sub PAGE_OFFSET { shift @_ & 0xFFF }

do {
    print "Enter AMD64 virtual address and I give you some information about it:\n";

    my $address = eval(readline);

    if($address == '') {
        exit;
    }

    print 'PML4 index:  ' . PML4_INDEX($address)  . "\n" .
          'PDP  index:  ' . PDP_INDEX($address)   . "\n" . 
          'PD   index:  ' . PD_INDEX($address)    . "\n" .
          'PT   index:  ' . PT_INDEX($address)    . "\n" .
          'Page offset: ' . PAGE_OFFSET($address) . "\n";

    printf("\nHex form: %016x\n" .
             "Dec form: %u\n"    .
             "Bin form: %064b\n", $address, $address, $address);

    print "\n\n";
} while(1);
