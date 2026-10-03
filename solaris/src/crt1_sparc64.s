! ====================================================================
! FTAGHN - Solaris 10 SPARC 64-bit C Runtime Startup (crt1)
! ====================================================================
    .section ".text"
    .global _start
    .type _start, #function
    .align 8

_start:
    clr     %fp
    ! On SPARC V9, stack pointer is biased by 2047 (0x7ff)
    ! Window save area is 128 bytes (16 * 8)
    ! Top of stack data starts at %sp + 2047 + 128 = %sp + 2175
    ldx     [%sp + 2175], %o0    ! argc (64-bit)
    add     %sp, 2183, %o1       ! argv (%sp + 2175 + 8)
    save    %sp, -192, %sp       ! allocate 192-byte frame (16-byte aligned)
    mov     %i0, %o0             ! pass argc to main
    mov     %i1, %o1             ! pass argv to main
    call    main
    nop
    call    exit                 ! exit(main(argc, argv))
    nop
    ta      0                    ! fallback halt/trap

