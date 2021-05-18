use utf8;
use strict;
use warnings;

use Test2::V0;
use LittleFox::LFOS::DSL;

my $parsed = LittleFox::LFOS::DSL->parse('syscalls {
    group scheduler {
        syscall sbrk {
            "Change where the program break is, the end of the heap -> this allocates memory"

            parameters {
                break: uint64_t "Where the userspace wants the new break"
            }
            returns {
                break: uint64_t "Where the new break is"
            }
        }
        syscall exit {
            "Exit the process calling this, this will not return"

            parameters {
                exit_code: uint16_t "Last wish from the dying process"
            }
        }
    }
}');

is($parsed->{type}, 'syscalls', 'Parsed to syscalls correctly');

my $group = $parsed->{groups}->[0];
is($group->{name}, 'scheduler', 'Parsed to syscalls and group name correct');

my $sbrk_call = $group->{calls}->[0];
ok(  $sbrk_call,                                                         'First syscall parsed');
like($sbrk_call->{desc},                        qr/^Change where the.*/, 'First syscall desc correct');
like($sbrk_call->{parameters}->{break}->{desc}, qr/^Where the user.*/, '  First syscall parameters desc correct');

done_testing;
