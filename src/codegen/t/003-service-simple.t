use utf8;
use strict;
use warnings;

use Test2::V0;
use LittleFox::LFOS::DSL;

my $parsed = LittleFox::LFOS::DSL->parse('service BlockDevice 703445e7-b014-4030-a4f6-3f2590580bdf {
    method open {
        parameters {
            device: string
        }
        returns {
            handle: uint64_t
        }
    }

    method close {
        parameters {
            handle: uint64_t
        }
    }
}');

is($parsed->{type}, 'service', 'Parsed to service correctly');

my $open_call = $parsed->{methods}->[0];
ok(  $open_call,                       'First method parsed');
is(  $open_call->{name},       'open', 'First method name correct');
like($open_call->{parameters}, { },    'First method parameters correct');
like($open_call->{returns},    { },    'First method returns correct');

my $close_call = $parsed->{methods}->[1];
ok(  $close_call,                        'Second method parsed');
is(  $close_call->{name},       'close', 'Second method name correct');
like($close_call->{parameters}, { },     'Second method parameters correct');
like($close_call->{returns},    { },     'Second method returns correct');

done_testing;
