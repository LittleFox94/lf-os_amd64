package LittleFox::LFOS::CodeGen;

use utf8;
use strict;
use warnings;

use FindBin;
use Mouse::Role;
use Mouse::Util::TypeConstraints;
use Scalar::Util;
use Template;
use Time::Piece;

use LittleFox::LFOS::CodeGen::Database;

has source => (
    is       => 'ro',
    isa      => 'HashRef',
    required => 1,
);

has source_file => (
    is       => 'ro',
    isa      => 'Str',
    required => 1,
);

has database => (
    is       => 'ro',
    isa      => 'LittleFox::LFOS::CodeGen::Database',
    required => 1,
);

has destination => (
    is       => 'ro',
    isa      => 'Str',
    required => 1,
);

subtype 'OutputMode'
    => as 'Str'
    => where { $_ eq 'headers' || $_ eq 'implementation' };

has output_mode => (
    is       => 'ro',
    isa      => 'OutputMode',
    required => 1,
);

subtype 'Side'
    => as 'Str'
    => where { $_ eq 'handler' || $_ eq 'caller' };

has side => (
    is       => 'ro',
    isa      => 'Side',
    required => 1,
);

has template => (
    is         => 'ro',
    isa        => 'Template',
    init_arg   => undef,
    builder    => '_build_template',
    lazy_build => 1,
);

requires 'run';

around new => sub {
    my ($orig, $self, %args) = @_;

    if($args{database}) {
        $args{database} = LittleFox::LFOS::CodeGen::Database->new(
            file => $args{database},
        );
    }

    $orig->($self, %args);
};

sub _build_template {
    my ($self) = @_;

    my $base_namespace = __PACKAGE__;
    my $pkg_name       = blessed($self);

    my $name;
    if(substr($pkg_name, 0, length($base_namespace)) eq $base_namespace) {
        $name =  lc(substr($pkg_name, length($base_namespace) + 2));
        $name =~ s/::/_/g;
    }
    elsif(my $name_func = $self->can('_name')) {
        $name = $name_func->($self);
    }
    else {
        die 'Code generators not in LittleFox::LFOS::CodeGen:: namespace must implement _name function for template loading!';
    }

    my $template_config = {
        STRICT       => 1,
        ENCODING     => 'utf8',
        INTERPOLATE  => 0,
        CACHE_SIZE   => 0,
        PRE_PROCESS  => 'header.tt',
        OUTPUT_PATH  => $self->destination,
        INCLUDE_PATH => [
            "$FindBin::Bin/template/",
            "$FindBin::Bin/template/$name/",
        ],
        VARIABLES => {
            output_mode => $self->output_mode,
            side        => $self->side,
            source_file => $self->source_file,
            source      => $self->source,
            timestamp   => scalar localtime,
        }
    };

    return Template->new($template_config);
}

no Mouse::Role;
1;
