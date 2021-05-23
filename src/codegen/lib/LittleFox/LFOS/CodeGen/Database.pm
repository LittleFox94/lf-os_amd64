package LittleFox::LFOS::CodeGen::Database;
# ABSTRACT: manages numeric IDs for nested Keys

our $VERSION = 0.001;

use utf8;
use strict;
use warnings;

use Mouse;
use MouseX::NativeTraits;
use List::Util 'all', 'max';
use YAML::XS 'LoadFile', 'DumpFile';

has file => (
    is       => 'ro',
    isa      => 'Str',
    required => 1,
);

has datastore => (
    is       => 'ro',
    isa      => 'HashRef',
    init_arg => '_datastore',
    traits   => ['Hash'],
);

around new => sub {
    my ($orig, $self, %args) = @_;

    my $ds = LoadFile($args{file});

    if($ds->{version} != $VERSION) {
        if(!$ds->{version}) {
            $ds->{version} = $VERSION;
        }
    }

    if(!$ds->{root}) {
        $ds->{root} = {};
    }

    $args{_datastore} = $ds;

    $self->$orig(%args);
};

sub DESTROY {
    my ($self) = @_;
    DumpFile($self->file, $self->datastore);
};

sub retrieve {
    my ($self, @keys) = @_;

    my $current = $self->datastore->{root};
    my @result;

    for my $key (@keys) {
        if($current->{$key}) {
            push(@result, $current->{$key}->{id});
            $current =    $current->{$key}->{children};
        }
        else {
            $current = undef;
        }

        if(!$current) {
            last;
        }
    }

    if($#result < $#keys) {
        for(my $i = $#result; $i < $#keys; ++$i) {
            push(@result, undef);
        }
    }

    return wantarray ? @result : \@result;
}

sub ensure {
    my ($self, @keys) = @_;
    my @cur = $self->retrieve(@keys);

    if(!all { defined } @cur) {
        my $current = $self->datastore->{root};

        for my $key (@keys) {
            $self->ensure_single($current, $key);

            $current = $current->{$key}->{children};
        }
    }

    return $self->retrieve(@keys);
}

sub ensure_single {
    my($self, $parent, $key) = @_;

    my $max_id   = max map { $parent->{$_}->{id} } keys $parent->%*;
       $max_id //= 0;
    my $new_id   = 1 + $max_id;

    $parent->{$key} //= {
        id       => $new_id,
        children => {},
    };

    $parent->{$key}->{children} //= {};
    $parent->{$key}->{id}       //= $new_id;
}

no Mouse;
1;
