! ====================================================================
! FTAGHN - Solaris 10 SPARC 32-bit C Runtime Startup (crt1)
! ====================================================================
    .section ".text"
    .global _start
    .type _start, #function
    .align 4

_start:
    clr     %fp
    ld      [%sp + 64], %o0      ! argc
    add     %sp, 68, %o1         ! argv
    save    %sp, -96, %sp        ! allocate 96-byte aligned frame
    mov     %i0, %o0             ! pass argc to main
    mov     %i1, %o1             ! pass argv to main
    call    main
    nop
    call    exit                 ! exit(main(argc, argv))
    nop
    ta      0                    ! fallback halt/trap

