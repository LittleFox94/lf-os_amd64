" Vim syntax file for LF declaration files (lfd) as used in LF OS for syscalls
" and services
" Maintainer: Mara Sophie Grosch <littlefox@lf-net.org>

if exists("b:current_syntax")
    finish
endif

syn match  lfdIdentifier        contained "\v[a-zA-Z_][a-zA-Z0-9_]*"
syn match  lfdUUID              contained "\v<[[:xdigit:]]{8}-[[:xdigit:]]{4}-[[:xdigit:]]{4}-[[:xdigit:]]{4}-[[:xdigit:]]{12}>"
syn match  lfdType              contained "\v(((struct|union)\s+\i+)|(bool|char|void|\i+_t))\**"
syn match  lfdStringPlaceholder contained "\v\$\w+"

syn match  lfdVar  "\v\i+\s*:\s*((struct|union)\s+)?\i+\**(\s+\"[^\"]*\")?;" contained contains=lfdIdentifier,lfdType,lfdString
syn region lfdVars start='{' end='}'                               contains=lfdVar

syn region lfdString start='"' end='"'  contains=lfdStringPlaceholder
syn region lfdBlock  start='{' end='}'  contains=lfdBlockStarters,lfdIdentBlockStarters,lfdIdentUUIDBLockStarters,lfdVar,lfdString

syn keyword lfdRootKeywords       syscalls
syn keyword lfdKeywords contained returns parameters service group syscall method union struct

syn match  lfdBlockStarters          "\v<(returns|parameters)"                       contains=lfdKeywords                       nextgroup=lfdVars
syn match  lfdIdentBlockStarters     "\v<(group|syscall|method|union|struct)\s+\i+>" contains=lfdKeywords,lfdIdentifier         nextgroup=lfdBlock
syn match  lfdIdentUUIDBLockStarters "\v<service\s+\i+\s+[-[:xdigit:]]{36}>"         contains=lfdKeywords,lfdIdentifier,lfdUUID nextgroup=lfdBlock

hi def link lfdRootKeywords      Keyword
hi def link lfdKeywords          Keyword
hi def link lfdIdentifier        Identifier
hi def link lfdType              Statement
hi def link lfdUUID              Constant
hi def link lfdString            String
hi def link lfdStringPlaceholder Identifier

let b:current_syntax='LF-decl'
