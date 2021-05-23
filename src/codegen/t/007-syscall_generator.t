use utf8;
use strict;
use warnings;

use Test2::V0 -target => 'LittleFox::LFOS::CodeGen::Syscall';

subtest type_bits => sub {
    my %type_tests = (
        'int8_t'  => 8,
        'void*'   => 64,
        'void***' => 64,
        'void'    => undef,
        'size_t'  => 64,
    );

    for my $type (keys %type_tests) {
        is($CLASS->type_bits($type), $type_tests{$type}, "type_bits for type '$type'");
    }
};

subtest register_alloc_vars => sub {
    subtest 'Fitting one register' => sub {
        my $res = $CLASS->register_alloc_vars(
            { name => 'a', type =>  'int8_t' },
            { name => 'b', type =>  'int8_t' },
            { name => 'c', type =>  'int8_t' },
            { name => 'd', type => 'int32_t' },
        );

        is(scalar (keys $res->{regs}->%*), 1, 'Correct amount of registers allocated');

        ok(    $res->{vars}->{a}->{register}
            eq $res->{vars}->{b}->{register}
            eq $res->{vars}->{c}->{register}
            eq $res->{vars}->{d}->{register}, 'Correctly mapped to same register');
    };

    subtest 'Not fitting one register' => sub {
        my $res = $CLASS->register_alloc_vars(
            { name => 'a', type =>  'int8_t' },
            { name => 'b', type => 'int16_t' },
            { name => 'c', type =>  'int8_t' },
            { name => 'd', type => 'int32_t' },
        );

        is(scalar (keys $res->{regs}->%*), 2, 'Correct amount of registers allocated');

        is($res->{vars}->{a}->{offset}, 0,  'Correct register offset');
        is($res->{vars}->{b}->{offset}, 16, 'Correct register offset');
        is($res->{vars}->{c}->{offset}, 32, 'Correct register offset');
        is($res->{vars}->{d}->{offset}, 0,  'Correct register offset');

        ok(    $res->{vars}->{a}->{register}
            eq $res->{vars}->{b}->{register}
            eq $res->{vars}->{c}->{register}, 'Correctly mapped to same register');

        ok(    $res->{vars}->{c}->{register}
            ne $res->{vars}->{d}->{register}, 'Correctly mapped to different register');
    };
};

subtest register_alloc => sub {
    my $res = $CLASS->register_alloc(
        {
            name       => 'syscall',
            parameters => [
                { name => 'a', type =>  'int8_t' },
                { name => 'b', type =>  'int8_t' },
                { name => 'c', type =>  'int8_t' },
                { name => 'd', type => 'int32_t' },
            ],
            returns => [
                { name => 'e', type =>  'int8_t' },
                { name => 'f', type => 'int64_t' },
            ],
        }
    );

    is(scalar keys $res->{regs}->%*, 2, 'Correct register count for syscall');

    is($res->{params}->{a}->{offset}, 0,  'Correct register offset for param a');
    is($res->{params}->{b}->{offset}, 8,  'Correct register offset for param b');
    is($res->{params}->{c}->{offset}, 16, 'Correct register offset for param c');
    is($res->{params}->{d}->{offset}, 32, 'Correct register offset for param d');

    ok(    $res->{params}->{a}->{register}
        eq $res->{params}->{b}->{register}
        eq $res->{params}->{c}->{register}
        eq $res->{params}->{d}->{register}, 'Correctly mapped to same register');

    is($res->{returns}->{e}->{offset}, 0,  'Correct register offset for return e');
    is($res->{returns}->{f}->{offset}, 0,  'Correct register offset for return f');

    ok(    $res->{returns}->{e}->{register}
        ne $res->{returns}->{f}->{register}, 'Correctly mapped to different register');
};

done_testing;
