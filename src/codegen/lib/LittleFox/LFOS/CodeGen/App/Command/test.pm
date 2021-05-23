package LittleFox::LFOS::CodeGen::App::Command::test;
# ABSTRACT: runs prove in correct work dir and with library path set

use utf8;
use strict;
use warnings;

use LittleFox::LFOS::CodeGen::App -command;
use App::Prove;

sub opt_spec {
    return (
        [ 'jobs|j=i',    'Jobs to run in parallel' ],
        [ 'timer',       'Show elapsed time after each test' ],
        [ 'formatter=s', 'Result formatter to use' ],
    );
}

sub execute {
    my ($self, $opt, $args) = @_;

    chdir($FindBin::Bin);

    my @args;
    push(@args, '-j',          $opt->{jobs})      if $opt->{jobs};
    push(@args, '--formatter', $opt->{formatter}) if $opt->{formatter};
    push(@args, '--timer')                        if $opt->{timer};

    my $prove = App::Prove->new;
    $prove->process_args(
        @args,
        '-I' => "$FindBin::Bin/lib",
        '-I' => "$FindBin::Bin/local/lib/perl5",
        '-r',
        't'
    );

    $prove->run;
}

1;
