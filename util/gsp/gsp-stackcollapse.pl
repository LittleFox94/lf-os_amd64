#!/usr/bin/env perl 

use utf8;
use strict;
use warnings;

my @current;
my @counter = (0);
my $prev_sym;

while(my $line = <>) {
    my ($sym, $with_offset) = $line =~ /^(\w+)(.*)$/;

    ++$counter[$#counter];

    if($with_offset) {
        if($prev_sym ne $sym) {
            # returned from function
            my $count = pop(@counter);
            print join(';', @current) . " $count\n";
            pop(@current);
        }

        $prev_sym = $sym;

        next;
    }
    elsif(@current) {
        my $count = pop(@counter);
        print join(';', @current) . " $count\n";
        push(@counter, 0);
    }

    push(@current, $sym);
    push(@counter, 0);

    $prev_sym = $sym;
}

while(my $sym = pop(@current)) {
    my $count = pop(@counter);
    print join(';', (@current, $sym)) . " $count\n";
    pop(@current);
}
