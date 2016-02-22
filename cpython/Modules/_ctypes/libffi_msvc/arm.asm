FFI_TYPE_VOID       EQU 0
FFI_TYPE_INT        EQU 1
FFI_TYPE_FLOAT      EQU 2
FFI_TYPE_DOUBLE     EQU 3
;FFI_TYPE_LONGDOUBLE EQU 4
FFI_TYPE_UINT8      EQU 5
FFI_TYPE_SINT8      EQU 6
FFI_TYPE_UINT16     EQU 7
FFI_TYPE_SINT16     EQU 8
FFI_TYPE_UINT32     EQU 9
FFI_TYPE_SINT32     EQU 10
FFI_TYPE_UINT64     EQU 11
FFI_TYPE_SINT64     EQU 12
FFI_TYPE_STRUCT     EQU 13
FFI_TYPE_POINTER    EQU 14

    AREA |.text|, CODE, THUMB, READONLY     ; .text

    ; r0:   ffi_prep_args
    ; r1:   &ecif
    ; r2:   cif->bytes
    ; r3:   fig->flags
    ; sp+0: ecif.rvalue
    ; sp+4: fn

    EXPORT |ffi_call_arm|

|ffi_call_arm| PROC

    ; Save registers
    stmfd sp!, {r4-r8, r10, fp, lr}
    mov   fp, sp

    mov   r4, r0        ; ffi_prep_args
    mov   r5, r1        ; &ecif
    mov   r6, r2        ; cif->bytes
    mov   r7, r3        ; fig->flags
    ldr   r8, [sp, #32] ; ecif.rvalue   
    ldr   r10, [sp, #36] ; fn

    ; Make room for all of the new args.
    sub   sp, sp, r2

    ; Place all of the ffi_prep_args in position
    mov   r0, sp
    ;     r1 already set

    ; And call
    blx   r4            ; ffi_prep_args

    ; move first 4 parameters in registers
    ldr   r0, [sp, #0]
    ldr   r1, [sp, #4]
    ldr   r2, [sp, #8]
    ldr   r3, [sp, #12]

    ; and adjust stack
    mov   ip, r6
    cmp   ip, #16
    blt   |$L1|
    movs  ip, #16
|$L1|
    add   sp, sp, ip

    ; call function
    blx   r10

    ; Remove the space we pushed for the args
    mov   sp, fp

    ; If the return value pointer is NULL, assume no return value.
    cmp   r8, #0
    beq   call_epilogue

; return INT
return_int
    cmp   r7, #FFI_TYPE_INT
    bne   return_float
    str   r0, [r8]
    beq   call_epilogue

; return FLOAT
return_float
|$L_float|
    cmp     r7, #FFI_TYPE_FLOAT
    bne     return_double
    vstr    s0, [r8]
    beq     call_epilogue

; return DOUBLE or LONGDOUBLE
return_double
|$L_double|
    cmp     r7, #FFI_TYPE_DOUBLE
    bne     return_sint64
    vstr    d0, [r8]
    beq     call_epilogue

; return SINT64 or UINT64
return_sint64
    cmp     r7, #FFI_TYPE_SINT64
    bne     call_epilogue
    str     a1, [r8]
    str     a2, [r8,#4]

call_epilogue
    mov     r0, #0
    ldmfd   sp!, {r4-r8, r10, fp, pc}

    ENDP

;;;;;;;;;;;;;; set_float_registerX ;;;;;;;;;;;;;;;

    EXPORT |set_float_register0|

|set_float_register0| PROC
    push    {lr}
    vldr    s0, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register1|

|set_float_register1| PROC
    push    {lr}
    vldr    s1, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register2|

|set_float_register2| PROC
    push    {lr}
    vldr    s2, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register3|

|set_float_register3| PROC
    push    {lr}
    vldr    s3, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register4|

|set_float_register4| PROC
    push    {lr}
    vldr    s4, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register5|

|set_float_register5| PROC
    push    {lr}
    vldr    s5, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register6|

|set_float_register6| PROC
    push    {lr}
    vldr    s6, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register7|

|set_float_register7| PROC
    push    {lr}
    vldr    s7, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register8|

|set_float_register8| PROC
    push    {lr}
    vldr    s8, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register9|

|set_float_register9| PROC
    push    {lr}
    vldr    s9, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register10|

|set_float_register10| PROC
    push    {lr}
    vldr    s10, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register11|

|set_float_register11| PROC
    push    {lr}
    vldr    s11, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register12|

|set_float_register12| PROC
    push    {lr}
    vldr    s12, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register13|

|set_float_register13| PROC
    push    {lr}
    vldr    s13, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register14|

|set_float_register14| PROC
    push    {lr}
    vldr    s14, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_float_register15|

|set_float_register15| PROC
    push    {lr}
    vldr    s15, [r0]
    pop     {pc}

    ENDP

;;;;;;;;;;;;;; set_double_registerX ;;;;;;;;;;;;;;;

    EXPORT |set_double_register0|

|set_double_register0| PROC
    push    {lr}
    vldr    d0, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_double_register1|

|set_double_register1| PROC
    push    {lr}
    vldr    d1, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_double_register2|

|set_double_register2| PROC
    push    {lr}
    vldr    d2, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_double_register3|

|set_double_register3| PROC
    push    {lr}
    vldr    d3, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_double_register4|

|set_double_register4| PROC
    push    {lr}
    vldr    d4, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_double_register5|

|set_double_register5| PROC
    push    {lr}
    vldr    d5, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_double_register6|

|set_double_register6| PROC
    push    {lr}
    vldr    d6, [r0]
    pop     {pc}

    ENDP

    EXPORT |set_double_register7|

|set_double_register7| PROC
    push    {lr}
    vldr    d7, [r0]
    pop     {pc}

    ENDP

    END
