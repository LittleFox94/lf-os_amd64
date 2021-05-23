use utf8;
use strict;
use warnings;

use Test2::V0;
use LittleFox::LFOS::DSL;

my $parsed = LittleFox::LFOS::DSL->parse('service BlockDevice 703445e7-b014-4030-a4f6-3f2590580bdf {
    struct ServiceDiscoverPayload {
        id: UUID
    }

    union Foo {
        sdp: ServiceDiscoverPayload;
        int: int
    }
}');

is($parsed->{type}, 'service', 'Parsed to service correctly');

my $sdp_type = $parsed->{types}->[0];
ok($sdp_type,                                   'First type parsed');
is($sdp_type->{name}, 'ServiceDiscoverPayload', 'First type name correct');
is($sdp_type->{type}, 'struct',                 'First type base type correct');

like($sdp_type->{members}, [ { name => 'id', type => 'UUID' } ],
    'First type members correct');

my $foo_type = $parsed->{types}->[1];
ok($foo_type,         'Second method parsed');
is($foo_type->{name}, 'Foo',   'Second method name correct');
is($foo_type->{type}, 'union', 'Second method parameters correct');

like($foo_type->{members}, [ { name => 'sdp', type => 'ServiceDiscoverPayload' }, { name => 'int', type => 'int' } ],
    'Second method returns correct');

done_testing;
