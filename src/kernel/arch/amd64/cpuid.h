#ifndef _CPUID_H_INCLUDED
#define _CPUID_H_INCLUDED

/**
 * Call CPUID for the given level, returning values for RAX to RDX.
 *
 * \param [in]  level Value to write to RAX before executing the CPUID instruction
 * \param [out] rax   Where to store result given in RAX, can be null
 * \param [out] rbx   Where to store result given in RBX, can be null
 * \param [out] rcx   Where to store result given in RCX, can be null
 * \param [out] rdx   Where to store result given in RDX, can be null
 */
static inline void cpuid(uint64_t level, uint64_t* rax, uint64_t* rbx, uint64_t* rcx, uint64_t* rdx) {
    uint64_t _rax, _rbx, _rcx, _rdx;
    asm(
        "cpuid"     :
        "=a"(_rax),
        "=b"(_rbx),
        "=c"(_rcx),
        "=d"(_rdx)  :
        "a"(level)
    );

    if(rax) *rax = _rax;
    if(rbx) *rbx = _rbx;
    if(rcx) *rcx = _rcx;
    if(rdx) *rdx = _rdx;
}

#endif
