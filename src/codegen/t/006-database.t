use utf8;
use strict;
use warnings;

use File::Temp 'tempfile';
use Test2::V0;

use LittleFox::LFOS::CodeGen::Database;

my ($tmpfh, $tmpfile) = tempfile('databaseXXXX', UNLINK => 1);
binmode($tmpfh, ':utf8');

print $tmpfh <<EOF;
---
version: 0.001
root:
  foo:
    id: 1
    children:
      bar:
        id: 2
        children: {}
EOF

close($tmpfh);

my $database = LittleFox::LFOS::CodeGen::Database->new(file => $tmpfile);
is($database->retrieve(qw(foo bar)), [ 1, 2 ],         'Key path with two defined elements correct');
is($database->retrieve(qw(foo baz)), [ 1, undef ],     'Key path with first defined, second undefined correct');
is($database->retrieve(qw(bar baz)), [ undef, undef ], 'Key path with all undefined has correct length');

is($database->ensure(qw(bar baz)), [ 2, 1 ], 'Ensuring completely fresh key path works');
is($database->ensure(qw(bar foo)), [ 2, 2 ], 'Ensuring child of existing key works');
is($database->ensure(qw(foo bar)), [ 1, 2 ], 'Ensuring existing key path works');

is($database->retrieve(qw(bar baz)), [ 2, 1 ], 'Retrieving after ensuring works');

my @longpath = qw(
    foo bar baz bla fasel
);
is($database->retrieve(@longpath), [ 1, 2, undef, undef, undef ], 'Retrieving long key path works');
is($database->ensure(@longpath),   [ 1, 2,     1,     1,     1 ], 'Ensuring long key path works');
is($database->retrieve(@longpath), [ 1, 2,     1,     1,     1 ], 'Retrieving long key path works');

done_testing;
