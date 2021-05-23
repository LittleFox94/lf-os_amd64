use utf8;
use strict;
use warnings;

use Test2::V0;
use LittleFox::LFOS::DSL;

my $parsed = LittleFox::LFOS::DSL->parse('syscalls {
    group scheduler {
        syscall sbrk {
            parameters {
                break: uint64_t
            }
            returns {
                break: uint64_t
            }
        }
        syscall exit {
            parameters {
                exit_code: uint16_t
            }
        }
        syscall mq_send {
            parameters {
                message: struct Message*;
            }
        }
    }
}');

is($parsed->{type}, 'syscalls', 'Parsed to syscalls correctly');

my $group = $parsed->{groups}->[0];
is($group->{name}, 'scheduler', 'Parsed to syscalls and group name correct');

my $sbrk_call = $group->{calls}->[0];
ok(  $sbrk_call,                                                            'First syscall parsed');
is(  $sbrk_call->{name},       'sbrk',                                      'First syscall name correct');
like($sbrk_call->{parameters}, [ { name => 'break', type => 'uint64_t' } ], 'First syscall parameters correct');
like($sbrk_call->{returns},    [ { name => 'break', type => 'uint64_t' } ], 'First syscall returns correct');

my $exit_call = $group->{calls}->[1];
ok(  $exit_call,                                                                'Second syscall parsed');
is(  $exit_call->{name},       'exit',                                          'Second syscall name correct');
like($exit_call->{parameters}, [ { name => 'exit_code', type => 'uint16_t' } ], 'Second syscall parameters correct');
like($exit_call->{returns},    [ ],                                             'Second syscall returns correct');

my $mq_call = $group->{calls}->[2];
ok(  $mq_call,                                                                     'Third syscall parsed');
is(  $mq_call->{name},       'mq_send',                                            'Third syscall name correct');
like($mq_call->{parameters}, [ { name => 'message', type => 'struct Message*' } ], 'Third syscall parameters correct');
like($mq_call->{returns},    [ ],                                                  'Third syscall returns correct');

done_testing;
