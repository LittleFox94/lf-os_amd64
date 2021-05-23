use utf8;
use strict;
use warnings;

use Test2::V0;
use LittleFox::LFOS::DSL;

my $parsed = LittleFox::LFOS::DSL->parse('something_strange {
    blurb foo {
    }
}');

like($parsed->[0], qr/Expected syscalls or service,.*/, 'Correct error catched');

done_testing;
