package LittleFox::LFOS::CodeGen::App::Command::code;
# ABSTRACT: Generates code from LF OS DSL

use utf8;
use strict;
use warnings;

use File::Slurp 'read_file';
use List::Util 'none';
use Ref::Util 'is_arrayref';

use LittleFox::LFOS::DSL;
use LittleFox::LFOS::CodeGen::Syscall;
use LittleFox::LFOS::CodeGen::Service;

use LittleFox::LFOS::CodeGen::App -command;

sub opt_spec {
    return (
        [ 'destination|d=s', 'Directory to store the generated files in' ],
        [ 'output|o=s',      'What code to generate (headers|implementation)' ],
        [ 'side|s=s',        'Side of the code to generate (handler|caller)' ],
        [ 'database|b=s',    'Database file for storing generated identifiers' ],
    );
}

sub usage_desc { "$0 code -b src/codegen.database.yml -d src/include/lfos -s headers src/services/*" }

sub validate_args {
    my ($self, $opt, $args) = @_;

    $self->usage_error('No output mode given')
        unless $opt->{output};

    if(none { $_ eq $opt->{output} } qw(headers implementation)) {
        $self->usage_error('Invalid output mode given');
    }

    $self->usage_error('No side given')
        unless $opt->{side};

    if(none { $_ eq $opt->{side} } qw(handler caller)) {
        $self->usage_error('Invalid side given');
    }

    $self->usage_error('No destination given')
        unless $opt->{destination};

    $self->usage_error('No database given')
        unless $opt->{database};

    $self->usage_error('No source given')
        unless scalar $args->@*;
}

sub execute {
    my ($self, $opt, $args) = @_;

    for my $source_file ($args->@*) {
        warn "Source file $source_file does not exist\n"
            unless -f $source_file;

        my $source = read_file($source_file);
        my $parsed = LittleFox::LFOS::DSL->parse($source);

        if(is_arrayref($parsed)) {
            for my $error ($parsed->@*) {
                print STDERR $error . "\n";
            }
        }

        my @generator_args = (
            source      => $parsed,
            source_file => $source_file,
            database    => $opt->{database},
            destination => $opt->{destination},
            output_mode => $opt->{output},
            side        => $opt->{side},
        );

        my $generator;
        if($parsed->{type} eq 'syscalls') {
            $generator = LittleFox::LFOS::CodeGen::Syscall->new(@generator_args);
        }
        elsif($parsed->{type} eq 'service') {
            $generator = LittleFox::LFOS::CodeGen::Service->new(@generator_args);
        }
        else {
            die "Don't know how to handle $parsed->{type} files\n";
        }

        $generator->run($parsed);
    }
}

1;
