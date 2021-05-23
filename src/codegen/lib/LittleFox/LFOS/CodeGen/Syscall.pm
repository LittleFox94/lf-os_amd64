package LittleFox::LFOS::CodeGen::Syscall;

use utf8;
use strict;
use warnings;

use Scalar::Util 'looks_like_number';
use List::Util 'any', 'none', 'sum', 'sum0', 'uniq';

use Mouse;
with 'LittleFox::LFOS::CodeGen';

# TODO: refactor out for different CPU archs
our %KNOWN_TYPES = (
        bool =>  1,
        char =>  8,
    _pointer => 64,

     int8_t  =>  8,
    uint8_t  =>  8,
    int16_t  => 16,
   uint16_t  => 16,
    int32_t  => 32,
   uint32_t  => 32,
    int64_t  => 64,
   uint64_t  => 64,
     size_t  => 64,
    ssize_t  => 64,

              pid_t => 'uint64_t',
            mq_id_t => 'uint64_t',
     pthread_cond_t => 'uint64_t',
    pthread_mutex_t => 'uint64_t',

                void => 0,
              uuid_t => 0,
    'struct Message' => 0,
);

# Registers defined by SysV ABI function parameters
#   - rcx, clobbered by `syscall`
#   + rax, defined to be used for return values
our %PARAM_REGS = (
    rdi => 64,
    rsi => 64,
    rax => 64,
    rdx => 64,
     r8 => 64,
     r9 => 64,
    r10 => 64,
);

# these registers are clobbered by `syscall`
our @CLOBBERED = qw(
    r11 rcx
);

sub type_bits {
    my ($self, $type) = @_;

    if($type =~ /\*/) {
        $type = '_pointer';
    }

    my $ret = $KNOWN_TYPES{$type};

    if(!looks_like_number($ret)) {
        return $self->type_bits($ret);
    }

    return $ret;
}

sub register_alloc_vars {
    my ($self, @vars) = @_;

    my $vars = {};

    my %regs;

    for my $var (@vars) {
        my $bits = $self->type_bits($var->{type});

        my $reg;
        my $reg_offset;
        my $reg_overhead;

        # first look if there is a good candidate in the already used registers
        # we want to clobber as less registers as possible
        for my $maybereg (keys %regs) {
            $reg_overhead = 0;
            my $maybereg_offset = $regs{$maybereg};

            # align on the amount of bits we need
            # so .. 8 bits on 0,  8, 16, 24, ...
            #      16 bits on 0, 16, 32, 48, ...
            #      32 bits on 0, 32, 64, ...
            if($maybereg_offset % $bits) {
                $reg_overhead     = $bits - ($maybereg_offset % $bits);
                $maybereg_offset += $reg_overhead;
            }

            # check if our var can fit
            if($PARAM_REGS{$maybereg} - $maybereg_offset >= $bits) {
                $reg        = $maybereg;
                $reg_offset = $maybereg_offset;
                last;
            }
        }

        if(!$reg) {
            # grab next free register thats large enough
            ($reg) = grep { $PARAM_REGS{$_} >= $bits }
                     grep { !$regs{$_} }
                     keys %PARAM_REGS;

            die "No register found to use for $var->{name}!\n" unless $reg;

            $reg_offset   = 0;
            $reg_overhead = 0;
        }

        $regs{$reg} += $bits + $reg_overhead;

        $vars->{$var->{name}} = {
            register => $reg,
            offset   => $reg_offset,
            size     => $bits,
        };
    }

    return {
        regs => \%regs,
        vars => $vars,
    }
}

sub register_alloc {
    my ($self, $call) = @_;

    my $bits_usable = sum values %PARAM_REGS;

    my $param_bits  = sum0 map { $self->type_bits($_->{type}) } $call->{parameters}->@*;
    my $return_bits = sum0 map { $self->type_bits($_->{type}) } $call->{returns}->@*;

    die "Not enough bits for parameters!\n"    if $param_bits  > $bits_usable;
    die "Not enough bits for return values!\n" if $return_bits > $bits_usable;

    my $params  = $self->register_alloc_vars($call->{parameters}->@*);
    my $returns = $self->register_alloc_vars($call->{returns}->@*);

    return {
        params  => $params->{vars},
        returns => $returns->{vars},
        regs    => {
            map {
                ( $_ => $PARAM_REGS{$_} )
            } uniq (keys $params->{regs}->%*, keys $returns->{regs}->%*)
        },
    }
}

sub run {
    my ($self) = @_;

    my @identifiers;

    # validation
    for my $group ($self->source->{groups}->@*) {
        $group->{id} = $self->database->ensure(
            qw(syscalls group),
            $group->{name}
        )->[2];

        for my $syscall ($group->{calls}->@*) {
            my $identifier = $group->{name} . '_' . $syscall->{name};

            if($self->side eq 'handler') {
                $identifier = 'sc_handle_' . $identifier;
            }
            elsif($self->side eq 'caller') {
                $identifier = 'sc_do_' . $identifier;
            }

            if(any { $_ eq $identifier } @identifiers) {
                die "Identifier $identifier not unique! They are built by combining group and syscall names (\$group_\$call) and have to be unique\n";
            }

            push(@identifiers, $identifier);

            my @types = map {
                my $type = $_->{type};
                $type =~ s/\*//g;
                $type;
            } (
                $syscall->{parameters}->@*,
                $syscall->{returns}->@*,
            );

            for my $type (@types) {
                if(none { $type eq $_ } keys %KNOWN_TYPES) {
                    die "Unknown type $type used\n";
                }
            }

            # we checked everything, augment it with more data

            $syscall->{id} = $self->database->ensure(
                qw(syscalls group),
                $group->{name}, $syscall->{name}
            )->[3];

            $syscall->{identifier} = $identifier;
            $syscall->{regalloc}   = $self->register_alloc($syscall);
        }
    }

    my $output = '';
    $self->template->process('syscalls.tt', {}, \$output)
        or die 'Error running template engine: ' . $self->template->error() . "\n";
    die $output;
}

no Mouse;
1;
