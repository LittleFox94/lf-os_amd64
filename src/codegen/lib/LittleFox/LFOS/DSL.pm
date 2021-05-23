package LittleFox::LFOS::DSL;

use utf8;
use strict;
use warnings;

use Regexp::Grammars;

sub _transform_vars {
    my $vars = shift;
    return [
        map {
            my $desc = $_->{Desc};
               $desc = substr($desc, 1, length($desc) - 2) if $desc;

            {
                name => $_->{Name},
                type => $_->{Type},
                desc => $desc,
            }
        } $vars->@*,
    ];
}

sub _transform_method {
    my $method = shift;

    my $desc = $method->{MethodBody}->{Desc};
       $desc = substr($desc, 1, length($desc) - 2) if $desc;

    return {
        name       => $method->{Identifier},
        desc       => $desc,
        parameters => _transform_vars($method->{MethodBody}->{Parameters}->{Var}),
        returns    => _transform_vars($method->{MethodBody}->{Returns}->{Var}),
    };
}

sub parse {
    my ($pkg, $source) = @_;

    my $parser = qr{
        <File>

        <rule: File> <Syscalls>
                   | <Service>
                   | <error:Expected syscalls or service>
        <token: BlockStart>       \{
        <token: BlockEnd>         \}
        <token: HexDigit>         [0-9a-fA-F]
        <token: Identifier>       [a-zA-Z_][a-zA-Z0-9_]+
        <token: TypeIdentifier>   ((struct|union)\s+)?[a-zA-Z_][a-zA-Z0-9_]+\**
        <token: String>           "[^"]*"
        <token: UUID>             <.HexDigit>{8}-<.HexDigit>{4}-<.HexDigit>{4}-<.HexDigit>{4}-<.HexDigit>{12}

        <rule:  Syscalls>        syscalls                       <.BlockStart> <[SyscallGroup]>*   <.BlockEnd>
        <rule:  SyscallGroup>    group      <Identifier>        <.BlockStart> (<[Syscall]>|<ws>)* <.BlockEnd>
        <rule:  Syscall>         syscall    <Identifier>        <.BlockStart> <MethodBody>        <.BlockEnd>

        <rule:  Service>         service    <Identifier> <UUID> <.BlockStart> <[ServiceBody]>*  <.BlockEnd>
        <rule:  ServiceBody>     <Method> | <Type>

        <rule:  Method>          method     <Identifier>        <.BlockStart> <MethodBody>      <.BlockEnd>
        <rule:  MethodBody>      <Desc=String>? <Parameters>? <Returns>?
        <rule:  Parameters>      parameters <.BlockStart> <[Var]>* %% ; <.BlockEnd>
        <rule:  Returns>         returns    <.BlockStart> <[Var]>* %% ; <.BlockEnd>

        <rule: Type>             <Type=Union> | <Type=Struct>
        <rule: Union>            union  <Type='union'>  <Identifier> <.BlockStart> <Desc=String>? <[Var]>* %% ; <.BlockEnd>
        <rule: Struct>           struct <Type='struct'> <Identifier> <.BlockStart> <Desc=String>? <[Var]>* %% ; <.BlockEnd>

        <rule:  Var>             <Name=Identifier>: <Type=TypeIdentifier> <Desc=String>?
    }xms;

    if($source =~ $parser) {
        my ($type) = grep { $_ } keys $/{File}->%*;

        my $data = $/{File}->{$type};

        if($type eq 'Syscalls') {
            return {
                type   => 'syscalls',
                groups => [ map {
                    {
                        name  => $_->{Identifier},
                        calls => [ map { _transform_method($_) } $_->{Syscall}->@* ],
                    }
                } $data->{SyscallGroup}->@*],
            }
        }
        elsif($type eq 'Service') {
            return {
                type => 'service',
                methods => [
                    map  { _transform_method($_->{Method}) }
                        grep { $_->{Method} }
                        $data->{ServiceBody}->@*
                ],
                types => [
                    map  { {
                        name    => $_->{Type}->{Type}->{Identifier},
                        type    => $_->{Type}->{Type}->{Type},
                        members => _transform_vars($_->{Type}->{Type}->{Var}),
                    } }
                    grep { $_->{Type} }
                    $data->{ServiceBody}->@*
                ],
            };
        }

        return \%/;
    }
    else {
        return \@!;
    }
}

1;
