	.file	"v.cpp"
	.intel_syntax noprefix
	.text
	.p2align 6
	.type	"_ZN12_GLOBAL__N_1L8scalar_hESt4spanIcLm18446744073709551615EEmSt17basic_string_viewIcSt11char_traitsIcEE.isra.0", @function
"_ZN12_GLOBAL__N_1L8scalar_hESt4spanIcLm18446744073709551615EEmSt17basic_string_viewIcSt11char_traitsIcEE.isra.0":
.LFB16392:
	.cfi_startproc
	add	rdx, rcx
	mov	r8, rdi
	mov	rax, rsi
	cmp	rcx, rdx
	je	.L4
	lea	r9, "_ZN8mustache9kEntitiesE"[rip]
	.p2align 5
	.p2align 6
	.p2align 3
.L3:
	movzx	esi, BYTE PTR [rcx]
	add	rcx, 1
	mov	rdi, QWORD PTR [r9+rsi*8]
	mov	QWORD PTR [r8+rax], rdi
	shr	rdi, 56
	add	rax, rdi
	cmp	rdx, rcx
	jne	.L3
	ret
	.p2align 6
	.p2align 3
.L4:
	ret
	.cfi_endproc
.LFE16392:
	.size	"_ZN12_GLOBAL__N_1L8scalar_hESt4spanIcLm18446744073709551615EEmSt17basic_string_viewIcSt11char_traitsIcEE.isra.0", .-"_ZN12_GLOBAL__N_1L8scalar_hESt4spanIcLm18446744073709551615EEmSt17basic_string_viewIcSt11char_traitsIcEE.isra.0"
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC5:
	.string	"basic_string_view::substr"
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC6:
	.string	"%s: __pos (which is %zu) > __size (which is %zu)"
	.section	.text.unlikely,"ax",@progbits
.LCOLDB14:
	.text
.LHOTB14:
	.p2align 6
	.type	"_ZN12_GLOBAL__N_1L9escaped_hESt4spanIcLm18446744073709551615EESt17basic_string_viewIcSt11char_traitsIcEE.isra.0", @function
"_ZN12_GLOBAL__N_1L9escaped_hESt4spanIcLm18446744073709551615EESt17basic_string_viewIcSt11char_traitsIcEE.isra.0":
.LFB16394:
	.cfi_startproc
	mov	r8, rdx
	cmp	rsi, 31
	jbe	.L23
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	eax, 572662306
	vpxor	xmm0, xmm0, xmm0
	mov	r11, rsi
	vpbroadcastd	ymm8, eax
	mov	eax, 656877351
	mov	r9, rdi
	xor	esi, esi
	vpbroadcastd	ymm7, eax
	mov	eax, 1044266558
	xor	r10d, r10d
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	push	r12
	vpbroadcastd	ymm6, eax
	push	rbx
	.cfi_offset 12, -24
	.cfi_offset 3, -32
	lea	rbx, "_ZN8mustache9kEntitiesE"[rip]
	and	rsp, -32
	sub	rsp, 96
	vmovdqu8	ZMMWORD PTR 32[rsp], zmm0
	mov	eax, 640034342
	vpbroadcastd	ymm9, eax
	mov	eax, 1010580540
	vpbroadcastd	ymm10, eax
	.p2align 6
	.p2align 3
.L15:
	vmovdqu	ymm0, YMMWORD PTR [r8+r10]
	vpcmpeqb	ymm1, ymm0, ymm8
	vpcmpeqb	ymm5, ymm0, ymm7
	vmovdqu	YMMWORD PTR [r9+rsi], ymm0
	vpcmpeqb	ymm3, ymm0, ymm10
	vpcmpeqb	ymm4, ymm0, ymm6
	vpcmpeqb	ymm2, ymm0, ymm9
	vpternlogq	ymm1, ymm5, ymm3, 254
	vpternlogq	ymm1, ymm4, ymm2, 254
	vpmovmskb	eax, ymm1
	test	eax, eax
	je	.L24
	xor	ecx, ecx
	lea	rdx, [r8+r10]
	popcnt	ecx, eax
	cmp	ecx, 8
	jg	.L25
	lea	r12, 64[r10]
	cmp	r11, r12
	jb	.L26
.L13:
	xor	edi, edi
	.p2align 6
	.p2align 6
	.p2align 3
.L14:
	xor	ecx, ecx
	tzcnt	ecx, eax
	add	rsi, rcx
	sub	rsi, rdi
	movzx	edi, BYTE PTR [rdx+rcx]
	mov	rdi, QWORD PTR [rbx+rdi*8]
	mov	QWORD PTR [r9+rsi], rdi
	shr	rdi, 56
	vmovdqu	ymm0, YMMWORD PTR 1[rdx+rcx]
	add	rsi, rdi
	lea	rdi, 1[rcx]
	blsr	eax, eax
	vmovdqu	YMMWORD PTR [r9+rsi], ymm0
	jne	.L14
	sub	rsi, rdi
	add	rsi, 32
.L10:
	add	r10, 32
	cmp	r11, r12
	jnb	.L15
	cmp	r11, r10
	jb	.L20
	sub	r11, r10
	lea	rdx, [r8+r10]
	mov	rdi, r9
	vzeroupper
	lea	rsp, -16[rbp]
	mov	rcx, rdx
	mov	rdx, r11
	pop	rbx
	.cfi_remember_state
	.cfi_restore 3
	pop	r12
	.cfi_restore 12
	pop	rbp
	.cfi_restore 6
	.cfi_def_cfa 7, 8
	jmp	"_ZN12_GLOBAL__N_1L8scalar_hESt4spanIcLm18446744073709551615EEmSt17basic_string_viewIcSt11char_traitsIcEE.isra.0"
	.p2align 6
	.p2align 3
.L24:
	.cfi_restore_state
	add	rsi, 32
	lea	r12, 64[r10]
	jmp	.L10
	.p2align 6
	.p2align 3
.L26:
	vmovdqa	YMMWORD PTR 32[rsp], ymm0
	vpxor	xmm0, xmm0, xmm0
	lea	rdx, 32[rsp]
	vmovdqa	YMMWORD PTR 64[rsp], ymm0
	jmp	.L13
	.p2align 6
	.p2align 3
.L25:
	cmp	r11, r10
	jb	.L19
	mov	rax, r11
	mov	ecx, 32
	mov	rdi, r9
	mov	QWORD PTR 16[rsp], r8
	sub	rax, r10
	mov	QWORD PTR 24[rsp], r9
	lea	r12, 64[r10]
	cmp	rax, rcx
	cmova	rax, rcx
	mov	rcx, rdx
	mov	rdx, rax
	call	"_ZN12_GLOBAL__N_1L8scalar_hESt4spanIcLm18446744073709551615EEmSt17basic_string_viewIcSt11char_traitsIcEE.isra.0"
	mov	r9, QWORD PTR 24[rsp]
	mov	r8, QWORD PTR 16[rsp]
	mov	rsi, rax
	jmp	.L10
	.p2align 6
	.p2align 3
.L23:
	.cfi_def_cfa 7, 8
	.cfi_restore 3
	.cfi_restore 6
	.cfi_restore 12
	mov	rdx, rsi
	mov	rcx, r8
	xor	esi, esi
	jmp	"_ZN12_GLOBAL__N_1L8scalar_hESt4spanIcLm18446744073709551615EEmSt17basic_string_viewIcSt11char_traitsIcEE.isra.0"
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.type	"_ZN12_GLOBAL__N_1L9escaped_hESt4spanIcLm18446744073709551615EESt17basic_string_viewIcSt11char_traitsIcEE.isra.0.cold", @function
"_ZN12_GLOBAL__N_1L9escaped_hESt4spanIcLm18446744073709551615EESt17basic_string_viewIcSt11char_traitsIcEE.isra.0.cold":
.LFSB16394:
.L19:
	.cfi_def_cfa 6, 16
	.cfi_offset 3, -32
	.cfi_offset 6, -16
	.cfi_offset 12, -24
	mov	rcx, r11
	mov	rdx, r10
	lea	rsi, .LC5[rip]
	xor	eax, eax
	lea	rdi, .LC6[rip]
	vzeroupper
	call	"_ZSt24__throw_out_of_range_fmtPKcz"@PLT
.L20:
	mov	rcx, r11
	mov	rdx, r10
	lea	rsi, .LC5[rip]
	xor	eax, eax
	lea	rdi, .LC6[rip]
	vzeroupper
	call	"_ZSt24__throw_out_of_range_fmtPKcz"@PLT
	.cfi_endproc
.LFE16394:
	.text
	.size	"_ZN12_GLOBAL__N_1L9escaped_hESt4spanIcLm18446744073709551615EESt17basic_string_viewIcSt11char_traitsIcEE.isra.0", .-"_ZN12_GLOBAL__N_1L9escaped_hESt4spanIcLm18446744073709551615EESt17basic_string_viewIcSt11char_traitsIcEE.isra.0"
	.section	.text.unlikely
	.size	"_ZN12_GLOBAL__N_1L9escaped_hESt4spanIcLm18446744073709551615EESt17basic_string_viewIcSt11char_traitsIcEE.isra.0.cold", .-"_ZN12_GLOBAL__N_1L9escaped_hESt4spanIcLm18446744073709551615EESt17basic_string_viewIcSt11char_traitsIcEE.isra.0.cold"
.LCOLDE14:
	.text
.LHOTE14:
	.section	.text._ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv,"axG",@progbits,_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv,comdat
	.align 2
	.p2align 6
	.weak	"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv"
	.type	"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv", @function
"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv":
.LFB12253:
	.cfi_startproc
	mov	rax, QWORD PTR [rdi]
	lea	rdx, 16[rdi]
	cmp	rax, rdx
	je	.L29
	mov	rsi, QWORD PTR 16[rdi]
	mov	rdi, rax
	add	rsi, 1
	jmp	"_ZdlPvm"@PLT
	.p2align 6
	.p2align 3
.L29:
	ret
	.cfi_endproc
.LFE12253:
	.size	"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv", .-"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv"
	.section	.text.startup,"ax",@progbits
	.p2align 6
	.globl	"main"
	.type	"main", @function
"main":
.LFB15005:
	.cfi_startproc
	movabs	rax, 8241989088176924002
	sub	rsp, 56
	.cfi_def_cfa_offset 64
	mov	QWORD PTR 38[rsp], rax
	mov	eax, 107
	mov	WORD PTR 46[rsp], ax
	lea	rax, 38[rsp]
	mov	DWORD PTR 12[rsp], edi
	mov	QWORD PTR 24[rsp], rax
	test	rsi, rsi
	je	.L35
.L31:
	mov	rdx, QWORD PTR "_ZN9benchmark16PrintDefaultHelpEv"@GOTPCREL[rip]
	lea	rdi, 12[rsp]
	mov	QWORD PTR [rsp], rsi
	call	"_ZN9benchmark10InitializeEPiPPcPFvvE"@PLT
	mov	rsi, QWORD PTR [rsp]
	mov	edi, DWORD PTR 12[rsp]
	call	"_ZN9benchmark27ReportUnrecognizedArgumentsEiPPc"@PLT
	test	al, al
	mov	eax, 1
	jne	.L30
	call	"_ZN9benchmark22RunSpecifiedBenchmarksEv"@PLT
	call	"_ZN9benchmark8ShutdownEv"@PLT
	xor	eax, eax
.L30:
	add	rsp, 56
	.cfi_remember_state
	.cfi_def_cfa_offset 8
	ret
.L35:
	.cfi_restore_state
	mov	DWORD PTR 12[rsp], 1
	lea	rsi, 24[rsp]
	jmp	.L31
	.cfi_endproc
.LFE15005:
	.size	"main", .-"main"
	.section	.text._ZNSt12_Vector_baseIlSaIlEED2Ev,"axG",@progbits,_ZNSt12_Vector_baseIlSaIlEED5Ev,comdat
	.align 2
	.p2align 6
	.weak	"_ZNSt12_Vector_baseIlSaIlEED2Ev"
	.type	"_ZNSt12_Vector_baseIlSaIlEED2Ev", @function
"_ZNSt12_Vector_baseIlSaIlEED2Ev":
.LFB15466:
	.cfi_startproc
	mov	rax, rdi
	mov	rdi, QWORD PTR [rdi]
	test	rdi, rdi
	je	.L38
	mov	rsi, QWORD PTR 16[rax]
	sub	rsi, rdi
	jmp	"_ZdlPvm"@PLT
	.p2align 6
	.p2align 3
.L38:
	ret
	.cfi_endproc
.LFE15466:
	.size	"_ZNSt12_Vector_baseIlSaIlEED2Ev", .-"_ZNSt12_Vector_baseIlSaIlEED2Ev"
	.weak	"_ZNSt12_Vector_baseIlSaIlEED1Ev"
	.set	"_ZNSt12_Vector_baseIlSaIlEED1Ev","_ZNSt12_Vector_baseIlSaIlEED2Ev"
	.section	.rodata._ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9push_backEc.str1.1,"aMS",@progbits,1
.LC15:
	.string	"basic_string::_M_create"
	.section	.text._ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9push_backEc,"axG",@progbits,_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9push_backEc,comdat
	.align 2
	.p2align 6
	.weak	"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9push_backEc"
	.type	"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9push_backEc", @function
"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9push_backEc":
.LFB15533:
	.cfi_startproc
	sub	rsp, 72
	.cfi_def_cfa_offset 80
	lea	rax, 16[rdi]
	mov	QWORD PTR 32[rsp], rbp
	.cfi_offset 6, -48
	mov	rbp, QWORD PTR 8[rdi]
	mov	QWORD PTR 40[rsp], r12
	.cfi_offset 12, -40
	mov	r12, QWORD PTR [rdi]
	mov	QWORD PTR 24[rsp], rbx
	.cfi_offset 3, -56
	mov	rbx, rdi
	mov	QWORD PTR 48[rsp], r13
	.cfi_offset 13, -32
	lea	r13, 1[rbp]
	mov	QWORD PTR 56[rsp], r14
	.cfi_offset 14, -24
	mov	r14d, esi
	cmp	r12, rax
	je	.L54
	mov	rcx, QWORD PTR 16[rdi]
	cmp	rcx, r13
	jb	.L55
.L44:
	mov	BYTE PTR [r12+rbp], r14b
	mov	rax, QWORD PTR [rbx]
	mov	BYTE PTR 1[rax+rbp], 0
	mov	rbp, QWORD PTR 32[rsp]
	mov	r12, QWORD PTR 40[rsp]
	mov	r14, QWORD PTR 56[rsp]
	mov	QWORD PTR 8[rbx], r13
	mov	rbx, QWORD PTR 24[rsp]
	mov	r13, QWORD PTR 48[rsp]
	add	rsp, 72
	.cfi_remember_state
	.cfi_def_cfa_offset 8
	ret
	.p2align 6
	.p2align 3
.L55:
	.cfi_restore_state
	movabs	rdi, 9223372036854775807
	mov	QWORD PTR 64[rsp], r15
	cmp	r13, rdi
	.cfi_offset 15, -16
	je	.L56
	lea	rdx, [rcx+rcx]
	mov	r15, rdx
	cmp	r13, rdx
	jb	.L57
	lea	rdi, 2[rbp]
	mov	r15, r13
.L47:
	mov	QWORD PTR 8[rsp], rcx
	call	"_Znwm"@PLT
	mov	rdx, rbp
	mov	rsi, r12
	mov	rdi, rax
	mov	QWORD PTR [rsp], rax
	call	"memcpy"@PLT
	mov	rcx, QWORD PTR 8[rsp]
.L48:
	mov	rdi, r12
	lea	rsi, 1[rcx]
	call	"_ZdlPvm"@PLT
	mov	r12, QWORD PTR [rsp]
.L49:
	mov	QWORD PTR 16[rbx], r15
	mov	r15, QWORD PTR 64[rsp]
	.cfi_remember_state
	.cfi_restore 15
	mov	QWORD PTR [rbx], r12
	jmp	.L44
	.p2align 6
	.p2align 3
.L57:
	.cfi_restore_state
	movabs	rax, 9223372036854775806
	cmp	rax, rdx
	jnb	.L58
	mov	r15, rax
	jmp	.L47
	.p2align 6
	.p2align 3
.L58:
	lea	rdi, 1[rdx]
	call	"_Znwm"@PLT
	mov	rdx, rbp
	mov	rsi, r12
	mov	rdi, rax
	mov	QWORD PTR [rsp], rax
	call	"memcpy"@PLT
	mov	rcx, QWORD PTR 16[rbx]
	jmp	.L48
	.p2align 6
	.p2align 3
.L54:
	.cfi_restore 15
	cmp	r13, 16
	jne	.L44
	mov	edi, 31
	mov	QWORD PTR 64[rsp], r15
	.cfi_offset 15, -16
	mov	r15d, 30
	call	"_Znwm"@PLT
	mov	rdx, QWORD PTR [r12]
	mov	QWORD PTR [rax], rdx
	mov	rdx, QWORD PTR 7[r12]
	mov	r12, rax
	mov	QWORD PTR 7[rax], rdx
	jmp	.L49
.L56:
	lea	rdi, .LC15[rip]
	call	"_ZSt20__throw_length_errorPKc"@PLT
	.cfi_endproc
.LFE15533:
	.size	"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9push_backEc", .-"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9push_backEc"
	.section	.text._ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE24_M_get_insert_unique_posERS7_,"axG",@progbits,_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE24_M_get_insert_unique_posERS7_,comdat
	.align 2
	.p2align 6
	.weak	"_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE24_M_get_insert_unique_posERS7_"
	.type	"_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE24_M_get_insert_unique_posERS7_", @function
"_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE24_M_get_insert_unique_posERS7_":
.LFB16184:
	.cfi_startproc
	push	r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	push	r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	push	r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	push	r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	push	rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	push	rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	sub	rsp, 24
	.cfi_def_cfa_offset 80
	mov	r15, QWORD PTR 16[rdi]
	mov	QWORD PTR [rsp], rdi
	mov	QWORD PTR 8[rsp], rsi
	test	r15, r15
	je	.L87
	mov	rax, QWORD PTR 8[rsp]
	mov	r13, QWORD PTR 32[r15]
	mov	rbx, QWORD PTR 40[r15]
	mov	rbp, QWORD PTR [rax]
	mov	r14, QWORD PTR 8[rax]
	jmp	.L68
	.p2align 6
	.p2align 3
.L88:
	mov	rdx, r12
	mov	rsi, r13
	mov	rdi, rbp
	call	"memcmp"@PLT
	test	eax, eax
	je	.L62
	test	eax, eax
	js	.L65
.L64:
	mov	rdx, QWORD PTR 24[r15]
	xor	esi, esi
	test	rdx, rdx
	je	.L67
.L89:
	mov	r13, QWORD PTR 32[rdx]
	mov	rbx, QWORD PTR 40[rdx]
	mov	r15, rdx
.L68:
	cmp	r14, rbx
	mov	r12, rbx
	cmovbe	r12, r14
	test	r12, r12
	jne	.L88
.L62:
	mov	rdx, r14
	sub	rdx, rbx
	cmp	rdx, 2147483647
	jg	.L64
	cmp	rdx, -2147483648
	jl	.L65
	mov	eax, edx
	test	edx, edx
	je	.L64
	test	eax, eax
	jns	.L64
.L65:
	mov	rdx, QWORD PTR 16[r15]
	mov	esi, 1
	test	rdx, rdx
	jne	.L89
.L67:
	mov	r9, r15
	test	sil, sil
	jne	.L61
.L69:
	test	r12, r12
	je	.L71
	mov	rdx, r12
	mov	rsi, rbp
	mov	rdi, r13
	mov	QWORD PTR [rsp], r9
	call	"memcmp"@PLT
	mov	r9, QWORD PTR [rsp]
	test	eax, eax
	jne	.L72
.L71:
	sub	rbx, r14
	cmp	rbx, 2147483647
	jg	.L74
	cmp	rbx, -2147483648
	jl	.L77
	mov	eax, ebx
	test	ebx, ebx
	jne	.L72
.L74:
	xor	r9d, r9d
.L70:
	add	rsp, 24
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	mov	rax, r15
	mov	rdx, r9
	pop	rbx
	.cfi_def_cfa_offset 48
	pop	rbp
	.cfi_def_cfa_offset 40
	pop	r12
	.cfi_def_cfa_offset 32
	pop	r13
	.cfi_def_cfa_offset 24
	pop	r14
	.cfi_def_cfa_offset 16
	pop	r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 6
	.p2align 3
.L87:
	.cfi_restore_state
	lea	r15, 8[rdi]
.L61:
	mov	rax, QWORD PTR [rsp]
	cmp	QWORD PTR 24[rax], r15
	je	.L76
	mov	rdi, r15
	call	"_ZSt18_Rb_tree_decrementPSt18_Rb_tree_node_base"@PLT
	mov	rcx, QWORD PTR 8[rsp]
	mov	r9, r15
	mov	rbx, QWORD PTR 40[rax]
	mov	r13, QWORD PTR 32[rax]
	mov	r15, rax
	mov	r14, QWORD PTR 8[rcx]
	mov	rbp, QWORD PTR [rcx]
	cmp	rbx, r14
	mov	r12, r14
	cmovbe	r12, rbx
	jmp	.L69
	.p2align 6
	.p2align 3
.L72:
	xor	edx, edx
	test	eax, eax
	cmovns	r9, rdx
	cmovs	r15, rdx
	jmp	.L70
	.p2align 6
	.p2align 3
.L76:
	mov	r9, r15
	xor	r15d, r15d
	jmp	.L70
.L77:
	xor	r15d, r15d
	jmp	.L70
	.cfi_endproc
.LFE16184:
	.size	"_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE24_M_get_insert_unique_posERS7_", .-"_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE24_M_get_insert_unique_posERS7_"
	.section	.text._ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorISA_ERS7_,"axG",@progbits,_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorISA_ERS7_,comdat
	.align 2
	.p2align 6
	.weak	"_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorISA_ERS7_"
	.type	"_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorISA_ERS7_", @function
"_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorISA_ERS7_":
.LFB16002:
	.cfi_startproc
	sub	rsp, 72
	.cfi_def_cfa_offset 80
	lea	rax, 8[rdi]
	mov	QWORD PTR 48[rsp], r13
	.cfi_offset 13, -32
	mov	r13, rdi
	mov	QWORD PTR 56[rsp], r14
	.cfi_offset 14, -24
	mov	r14, rdx
	cmp	rsi, rax
	je	.L139
	mov	QWORD PTR 32[rsp], rbp
	.cfi_offset 6, -48
	mov	rbp, QWORD PTR 40[rsi]
	mov	QWORD PTR 40[rsp], r12
	.cfi_offset 12, -40
	mov	r12, QWORD PTR 8[rdx]
	mov	QWORD PTR 64[rsp], r15
	.cfi_offset 15, -16
	mov	r15, QWORD PTR [rdx]
	mov	rdx, rbp
	cmp	r12, rbp
	mov	QWORD PTR 24[rsp], rbx
	.cfi_offset 3, -56
	mov	rbx, rsi
	cmovbe	rdx, r12
	test	rdx, rdx
	je	.L140
	mov	rcx, QWORD PTR 32[rsi]
	mov	rdi, r15
	mov	QWORD PTR 8[rsp], rdx
	mov	rsi, rcx
	mov	QWORD PTR [rsp], rcx
	call	"memcmp"@PLT
	mov	rcx, QWORD PTR [rsp]
	mov	rdx, QWORD PTR 8[rsp]
	test	eax, eax
	je	.L141
.L101:
	test	eax, eax
	jns	.L102
.L99:
	cmp	QWORD PTR 24[r13], rbx
	je	.L142
	mov	rdi, rbx
	call	"_ZSt18_Rb_tree_decrementPSt18_Rb_tree_node_base"@PLT
	mov	rdx, r12
	mov	rbp, QWORD PTR 40[rax]
	mov	rcx, rax
	cmp	rbp, r12
	cmovbe	rdx, rbp
	test	rdx, rdx
	je	.L104
	mov	rdi, QWORD PTR 32[rax]
	mov	rsi, r15
	mov	QWORD PTR [rsp], rax
	call	"memcmp"@PLT
	mov	rcx, QWORD PTR [rsp]
	test	eax, eax
	je	.L104
.L105:
	test	eax, eax
	jns	.L136
.L106:
	mov	rax, rbx
	mov	rdx, rbx
	cmp	QWORD PTR 24[rcx], 0
	je	.L143
.L137:
	mov	r15, QWORD PTR 64[rsp]
	.cfi_remember_state
	.cfi_restore 15
	jmp	.L125
	.p2align 6
	.p2align 3
.L140:
	.cfi_restore_state
	mov	rax, r12
	sub	rax, rbp
	cmp	rax, 2147483647
	jg	.L98
	cmp	rax, -2147483648
	jl	.L99
	test	eax, eax
	je	.L98
	js	.L99
	.p2align 6
	.p2align 3
.L98:
	sub	rbp, r12
	cmp	rbp, 2147483647
	jg	.L109
	cmp	rbp, -2147483648
	jl	.L110
	mov	eax, ebp
	test	ebp, ebp
	jne	.L108
.L109:
	mov	r15, QWORD PTR 64[rsp]
	.cfi_restore 15
	mov	rax, rbx
	xor	edx, edx
.L125:
	mov	rbx, QWORD PTR 24[rsp]
	.cfi_restore 3
	mov	rbp, QWORD PTR 32[rsp]
	.cfi_restore 6
	mov	r12, QWORD PTR 40[rsp]
	.cfi_restore 12
	mov	r13, QWORD PTR 48[rsp]
	mov	r14, QWORD PTR 56[rsp]
	add	rsp, 72
	.cfi_def_cfa_offset 8
	ret
	.p2align 6
	.p2align 3
.L141:
	.cfi_def_cfa_offset 80
	.cfi_offset 3, -56
	.cfi_offset 6, -48
	.cfi_offset 12, -40
	.cfi_offset 15, -16
	mov	rsi, r12
	sub	rsi, rbp
	cmp	rsi, 2147483647
	jg	.L102
	cmp	rsi, -2147483648
	jl	.L99
	mov	eax, esi
	test	esi, esi
	jne	.L101
.L102:
	mov	rsi, r15
	mov	rdi, rcx
	call	"memcmp"@PLT
	test	eax, eax
	je	.L98
.L108:
	test	eax, eax
	jns	.L109
.L110:
	cmp	QWORD PTR 32[r13], rbx
	je	.L138
	mov	rdi, rbx
	call	"_ZSt18_Rb_tree_incrementPSt18_Rb_tree_node_base"@PLT
	mov	rdx, r12
	mov	rcx, QWORD PTR 40[rax]
	mov	rbp, rax
	cmp	rcx, r12
	cmovbe	rdx, rcx
	test	rdx, rdx
	je	.L112
	mov	rsi, QWORD PTR 32[rax]
	mov	rdi, r15
	mov	QWORD PTR [rsp], rcx
	call	"memcmp"@PLT
	mov	rcx, QWORD PTR [rsp]
	test	eax, eax
	jne	.L113
.L112:
	sub	r12, rcx
	cmp	r12, 2147483647
	jg	.L136
	cmp	r12, -2147483648
	jl	.L114
	mov	eax, r12d
	test	r12d, r12d
	je	.L136
.L113:
	test	eax, eax
	jns	.L136
.L114:
	mov	rax, rbp
	mov	rdx, rbp
	cmp	QWORD PTR 24[rbx], 0
	jne	.L137
.L138:
	mov	r15, QWORD PTR 64[rsp]
	.cfi_restore 15
	xor	eax, eax
	mov	rdx, rbx
	jmp	.L125
	.p2align 6
	.p2align 3
.L139:
	.cfi_restore 3
	.cfi_restore 6
	.cfi_restore 12
	cmp	QWORD PTR 40[rdi], 0
	je	.L92
	mov	QWORD PTR 40[rsp], r12
	.cfi_offset 12, -40
	mov	r12, QWORD PTR 32[rdi]
	mov	QWORD PTR 24[rsp], rbx
	mov	QWORD PTR 32[rsp], rbp
	.cfi_offset 3, -56
	.cfi_offset 6, -48
	mov	rbx, QWORD PTR 40[r12]
	mov	rbp, QWORD PTR 8[rdx]
	cmp	rbx, rbp
	mov	rdx, rbp
	cmovbe	rdx, rbx
	test	rdx, rdx
	je	.L93
	mov	rsi, QWORD PTR [r14]
	mov	rdi, QWORD PTR 32[r12]
	call	"memcmp"@PLT
	test	eax, eax
	jne	.L94
.L93:
	sub	rbx, rbp
	cmp	rbx, 2147483647
	jg	.L129
	cmp	rbx, -2147483648
	jl	.L95
	mov	eax, ebx
	test	ebx, ebx
	jne	.L94
.L129:
	mov	rbx, QWORD PTR 24[rsp]
	.cfi_restore 3
	mov	rbp, QWORD PTR 32[rsp]
	.cfi_restore 6
	mov	r12, QWORD PTR 40[rsp]
	.cfi_restore 12
.L92:
	mov	rsi, r14
	mov	rdi, r13
	mov	r14, QWORD PTR 56[rsp]
	mov	r13, QWORD PTR 48[rsp]
	add	rsp, 72
	.cfi_def_cfa_offset 8
	jmp	"_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE24_M_get_insert_unique_posERS7_"
	.p2align 6
	.p2align 3
.L104:
	.cfi_def_cfa_offset 80
	.cfi_offset 3, -56
	.cfi_offset 6, -48
	.cfi_offset 12, -40
	.cfi_offset 15, -16
	sub	rbp, r12
	cmp	rbp, 2147483647
	jg	.L136
	cmp	rbp, -2147483648
	jl	.L106
	mov	eax, ebp
	test	ebp, ebp
	jne	.L105
.L136:
	mov	rbx, QWORD PTR 24[rsp]
	.cfi_restore 3
	mov	rbp, QWORD PTR 32[rsp]
	.cfi_restore 6
	mov	r12, QWORD PTR 40[rsp]
	.cfi_restore 12
	mov	r15, QWORD PTR 64[rsp]
	.cfi_restore 15
	jmp	.L92
	.p2align 6
	.p2align 3
.L94:
	.cfi_offset 3, -56
	.cfi_offset 6, -48
	.cfi_offset 12, -40
	test	eax, eax
	jns	.L129
.L95:
	xor	eax, eax
	mov	rdx, r12
	jmp	.L125
	.p2align 6
	.p2align 3
.L142:
	.cfi_offset 15, -16
	mov	r15, QWORD PTR 64[rsp]
	.cfi_remember_state
	.cfi_restore 15
	mov	rax, rbx
	mov	rdx, rbx
	jmp	.L125
	.p2align 6
	.p2align 3
.L143:
	.cfi_restore_state
	mov	r15, QWORD PTR 64[rsp]
	.cfi_restore 15
	xor	eax, eax
	mov	rdx, rcx
	jmp	.L125
	.cfi_endproc
.LFE16002:
	.size	"_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorISA_ERS7_", .-"_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorISA_ERS7_"
	.section	.rodata.str1.8
	.align 8
.LC17:
	.string	"int64_t benchmark::State::range(std::size_t) const"
	.align 8
.LC18:
	.string	"/usr/include/benchmark/benchmark.h"
	.section	.rodata.str1.1
.LC19:
	.string	"range_.size() > pos"
	.section	.rodata.str1.8
	.align 8
.LC21:
	.string	"cannot create std::vector larger than max_size()"
	.align 8
.LC22:
	.string	"benchmark::State::StateIterator& benchmark::State::StateIterator::operator++()"
	.section	.rodata.str1.1
.LC23:
	.string	"cached_ > 0"
.LC20:
	.string	"<>&\"'"
	.section	.text.unlikely
.LCOLDB28:
	.text
.LHOTB28:
	.p2align 6
	.type	"_ZN12_GLOBAL__N_1L3armERN9benchmark5StateE", @function
"_ZN12_GLOBAL__N_1L3armERN9benchmark5StateE":
.LFB14964:
	.cfi_startproc
	.cfi_personality 0x9b,"DW.ref.__gxx_personality_v0"
	.cfi_lsda 0x1b,.LLSDA14964
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	vpxor	xmm0, xmm0, xmm0
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	push	r15
	push	r14
	push	r13
	push	r12
	push	rbx
	sub	rsp, 200
	.cfi_offset 15, -24
	.cfi_offset 14, -32
	.cfi_offset 13, -40
	.cfi_offset 12, -48
	.cfi_offset 3, -56
	mov	rcx, QWORD PTR 40[rdi]
	mov	rax, QWORD PTR 32[rdi]
	mov	QWORD PTR 16[rsp], rdi
	mov	rdx, rcx
	mov	QWORD PTR 88[rsp], rcx
	sub	rdx, rax
	vmovdqu	YMMWORD PTR 96[rsp], ymm0
	cmp	rdx, 8
	jle	.L271
	mov	rbx, QWORD PTR [rax]
	mov	r13, QWORD PTR 8[rax]
	lea	rax, 112[rsp]
	mov	BYTE PTR 112[rsp], 0
	mov	QWORD PTR 96[rsp], rax
	mov	QWORD PTR 104[rsp], 0
	test	rbx, rbx
	je	.L217
	xor	r15d, r15d
	mov	r14d, 12345
	lea	r12, 96[rsp]
	vzeroupper
	jmp	.L149
	.p2align 6
	.p2align 3
.L284:
	mov	edx, r14d
	mov	ecx, 3435973837
	shr	edx, 8
	mov	eax, edx
	imul	rax, rcx
	shr	rax, 34
	lea	eax, [rax+rax*4]
	sub	edx, eax
	lea	rax, .LC20[rip]
	movzx	esi, BYTE PTR [rax+rdx]
.L148:
	movsx	esi, sil
	mov	rdi, r12
.LEHB0:
	call	"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9push_backEc"
.LEHE0:
	add	r15, 1
	cmp	rbx, r15
	je	.L283
.L149:
	imul	ecx, r14d, 1664525
	lea	r14d, 1013904223[rcx]
	mov	edx, r14d
	shr	edx, 16
	mov	eax, edx
	imul	rax, rax, 1374389535
	shr	rax, 37
	imul	eax, eax, 100
	sub	edx, eax
	cmp	rdx, r13
	jb	.L284
	mov	esi, r14d
	shr	esi, 20
	mov	eax, esi
	imul	rax, rax, 1321528399
	shr	rax, 35
	imul	eax, eax, 26
	sub	esi, eax
	add	esi, 97
	jmp	.L148
	.p2align 6
	.p2align 3
.L283:
	mov	rbx, QWORD PTR 104[rsp]
	lea	r13, [rbx+rbx*2]
	mov	r14, rbx
	add	r13, r13
	mov	rax, r13
	add	rax, 32
	mov	QWORD PTR [rsp], rax
	js	.L272
	jne	.L146
	mov	rax, QWORD PTR 96[rsp]
	vpxor	xmm0, xmm0, xmm0
	lea	r12, 144[rsp]
	mov	QWORD PTR 48[rsp], 0
	vmovdqu	YMMWORD PTR 128[rsp], ymm0
	lea	rcx, [rax+rbx]
	mov	QWORD PTR 128[rsp], r12
	mov	QWORD PTR 24[rsp], rcx
	mov	QWORD PTR 8[rsp], 0
	cmp	rcx, rax
	je	.L285
.L215:
	mov	QWORD PTR 56[rsp], rax
	vzeroupper
	.p2align 6
	.p2align 3
.L167:
	mov	rax, QWORD PTR 56[rsp]
	lea	rcx, "_ZN8mustache9kEntitiesE"[rip]
	movzx	eax, BYTE PTR [rax]
	mov	rax, QWORD PTR [rcx+rax*8]
	mov	r14, rax
	mov	QWORD PTR 88[rsp], rax
	shr	r14, 56
	je	.L155
	mov	r13, QWORD PTR 136[rsp]
	xor	r15d, r15d
	lea	rax, [r14+r13]
	mov	QWORD PTR 80[rsp], rax
	jmp	.L166
	.p2align 6
	.p2align 3
.L158:
	shrx	rax, QWORD PTR 88[rsp], r15
	mov	BYTE PTR [r8+r14], al
	add	r15d, 8
	mov	rax, QWORD PTR 128[rsp]
	mov	BYTE PTR [rax+r13], 0
	mov	QWORD PTR 136[rsp], r13
	cmp	r13, QWORD PTR 80[rsp]
	je	.L155
.L166:
	mov	r8, QWORD PTR 128[rsp]
	mov	r14, r13
	add	r13, 1
	cmp	r8, r12
	je	.L286
	mov	rbx, QWORD PTR 144[rsp]
	cmp	rbx, r13
	jnb	.L158
	movabs	rax, 9223372036854775807
	cmp	r13, rax
	je	.L273
	lea	rax, [rbx+rbx]
	mov	QWORD PTR 72[rsp], rax
	cmp	r13, rax
	jb	.L161
	mov	QWORD PTR 72[rsp], r13
	lea	rdi, 2[r14]
.L157:
	mov	QWORD PTR 64[rsp], r8
.LEHB1:
	call	"_Znwm"@PLT
.LEHE1:
	mov	rsi, QWORD PTR 64[rsp]
	mov	rdx, r14
	mov	rdi, rax
	mov	rbx, rax
	call	"memcpy"@PLT
	mov	r8, QWORD PTR 64[rsp]
	cmp	r8, r12
	je	.L165
	mov	rax, QWORD PTR 144[rsp]
	mov	rdi, r8
	lea	rsi, 1[rax]
	call	"_ZdlPvm"@PLT
.L165:
	mov	rax, QWORD PTR 72[rsp]
	mov	QWORD PTR 128[rsp], rbx
	mov	r8, rbx
	mov	QWORD PTR 144[rsp], rax
	jmp	.L158
.L217:
	mov	QWORD PTR [rsp], 32
	xor	r13d, r13d
	vzeroupper
.L146:
	mov	r15, QWORD PTR [rsp]
	lea	r12, 96[rsp]
	mov	rdi, r15
.LEHB2:
	call	"_Znwm"@PLT
.LEHE2:
	mov	BYTE PTR [rax], 0
	lea	rcx, [rax+r15]
	lea	rdx, 31[r13]
	xor	esi, esi
	lea	rdi, 1[rax]
	mov	QWORD PTR 8[rsp], rcx
	lea	r12, 144[rsp]
	mov	QWORD PTR 48[rsp], rax
	call	"memset"@PLT
	mov	rax, QWORD PTR 96[rsp]
	vpxor	xmm0, xmm0, xmm0
	vmovdqu	YMMWORD PTR 128[rsp], ymm0
	lea	rcx, [rax+rbx]
	mov	QWORD PTR 128[rsp], r12
	mov	QWORD PTR 24[rsp], rcx
	cmp	rax, rcx
	jne	.L215
	mov	rbx, QWORD PTR [rsp]
	vzeroupper
.L154:
	mov	rdi, rbx
.LEHB3:
	call	"_Znwm"@PLT
.LEHE3:
	mov	BYTE PTR [rax], 0
	mov	r14, QWORD PTR 104[rsp]
	mov	r13, rax
	cmp	rbx, 1
	jne	.L169
.L280:
	mov	rax, QWORD PTR 96[rsp]
.L168:
	mov	rdx, rax
	mov	rsi, r14
	mov	rdi, r13
.LEHB4:
	call	"_ZN12_GLOBAL__N_1L9escaped_hESt4spanIcLm18446744073709551615EESt17basic_string_viewIcSt11char_traitsIcEE.isra.0"
	cmp	rax, QWORD PTR 136[rsp]
	je	.L287
.L170:
	vpxor	xmm0, xmm0, xmm0
	mov	edi, 26
	vmovdqu	YMMWORD PTR 160[rsp], ymm0
	vzeroupper
	call	"_Znwm"@PLT
.LEHE4:
	vmovdqa	xmm0, XMMWORD PTR .LC25[rip]
	mov	BYTE PTR 25[rax], 0
	lea	rsi, 160[rsp]
	mov	rdi, QWORD PTR 16[rsp]
	mov	QWORD PTR 160[rsp], rax
	vmovdqu	XMMWORD PTR [rax], xmm0
	vmovdqa	xmm0, XMMWORD PTR .LC26[rip]
	mov	QWORD PTR 176[rsp], 25
	vmovdqu	XMMWORD PTR 9[rax], xmm0
	mov	QWORD PTR 168[rsp], 25
.LEHB5:
	call	"_ZN9benchmark5State13SkipWithErrorERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE"@PLT
.LEHE5:
	mov	rdi, QWORD PTR 160[rsp]
	lea	rax, 176[rsp]
	cmp	rdi, rax
	je	.L176
	mov	rax, QWORD PTR 176[rsp]
	lea	rsi, 1[rax]
	call	"_ZdlPvm"@PLT
.L176:
	test	r13, r13
	je	.L177
	mov	rsi, rbx
	mov	rdi, r13
	call	"_ZdlPvm"@PLT
.L177:
	mov	rdi, QWORD PTR 128[rsp]
	cmp	rdi, r12
	je	.L178
	mov	rax, QWORD PTR 144[rsp]
	lea	rsi, 1[rax]
	call	"_ZdlPvm"@PLT
.L178:
	mov	rax, QWORD PTR 48[rsp]
	test	rax, rax
	je	.L207
	mov	rsi, QWORD PTR [rsp]
	mov	rdi, rax
	call	"_ZdlPvm"@PLT
	.p2align 6
	.p2align 3
.L207:
	mov	rdi, QWORD PTR 96[rsp]
	lea	rax, 112[rsp]
	cmp	rdi, rax
	je	.L278
	mov	rax, QWORD PTR 112[rsp]
	lea	rsi, 1[rax]
	call	"_ZdlPvm"@PLT
.L278:
	add	rsp, 200
	pop	rbx
	pop	r12
	pop	r13
	pop	r14
	pop	r15
	pop	rbp
	.cfi_remember_state
	.cfi_def_cfa 7, 8
	ret
	.p2align 6
	.p2align 3
.L286:
	.cfi_restore_state
	cmp	r13, 16
	jne	.L158
	mov	QWORD PTR 72[rsp], 30
	mov	edi, 31
	jmp	.L157
	.p2align 6
	.p2align 3
.L155:
	add	QWORD PTR 56[rsp], 1
	mov	rax, QWORD PTR 56[rsp]
	cmp	QWORD PTR 24[rsp], rax
	jne	.L167
	mov	rbx, QWORD PTR 8[rsp]
	sub	rbx, QWORD PTR 48[rsp]
	jne	.L154
	mov	r14, QWORD PTR 104[rsp]
	mov	rax, QWORD PTR 96[rsp]
.L216:
	xor	r13d, r13d
	jmp	.L168
	.p2align 6
	.p2align 3
.L161:
	movabs	rax, 9223372036854775806
	mov	rcx, QWORD PTR 72[rsp]
	cmp	rax, rcx
	jnb	.L288
	movabs	rax, 9223372036854775806
	movabs	rdi, 9223372036854775807
	mov	QWORD PTR 72[rsp], rax
	jmp	.L157
.L169:
	lea	rdx, -1[rbx]
	lea	rdi, 1[rax]
	xor	esi, esi
	call	"memset"@PLT
	jmp	.L280
.L287:
	mov	r14, QWORD PTR 128[rsp]
	test	rax, rax
	je	.L289
	mov	rdx, rax
	mov	rsi, r14
	mov	rdi, r13
	call	"memcmp"@PLT
	test	eax, eax
	jne	.L170
.L175:
	mov	rsi, rbx
	mov	rdi, r13
	call	"_ZdlPvm"@PLT
.L281:
	cmp	r14, r12
	je	.L173
	mov	rax, QWORD PTR 144[rsp]
	mov	rdi, r14
	lea	rsi, 1[rax]
	call	"_ZdlPvm"@PLT
.L173:
	mov	rdi, QWORD PTR 16[rsp]
	mov	eax, DWORD PTR 28[rdi]
	test	eax, eax
	je	.L181
.LEHB6:
	call	"_ZN9benchmark5State16StartKeepRunningEv"@PLT
.L182:
	mov	rdi, QWORD PTR 16[rsp]
	call	"_ZN9benchmark5State17FinishKeepRunningEv"@PLT
	mov	rcx, QWORD PTR 16[rsp]
	cmp	BYTE PTR 24[rcx], 0
	je	.L221
	mov	rax, QWORD PTR 16[rcx]
	sub	rax, QWORD PTR [rcx]
	vxorps	xmm0, xmm0, xmm0
	add	rax, QWORD PTR 8[rcx]
	imul	rax, QWORD PTR 104[rsp]
	vcvtsi2sd	xmm0, xmm0, rax
	vmovlpd	QWORD PTR 80[rsp], xmm0
.L189:
	vpxor	xmm0, xmm0, xmm0
	mov	edi, 17
	vmovdqu	YMMWORD PTR 160[rsp], ymm0
	vzeroupper
	call	"_Znwm"@PLT
.LEHE6:
	vmovdqa	xmm0, XMMWORD PTR .LC27[rip]
	mov	QWORD PTR 160[rsp], rax
	mov	r12, rax
	mov	BYTE PTR 16[rax], 0
	vmovdqu	XMMWORD PTR [rax], xmm0
	mov	rax, QWORD PTR 16[rsp]
	mov	QWORD PTR 176[rsp], 16
	mov	r13, QWORD PTR 80[rax]
	lea	r15, 72[rax]
	mov	QWORD PTR 168[rsp], 16
	mov	QWORD PTR 88[rsp], r15
	test	r13, r13
	jne	.L195
	jmp	.L222
	.p2align 6
	.p2align 3
.L292:
	mov	edx, 16
	mov	rdi, QWORD PTR 32[r13]
	mov	rsi, r12
	cmp	r14, rdx
	cmovbe	rdx, r14
	call	"memcmp"@PLT
	test	eax, eax
	je	.L290
.L192:
	test	eax, eax
	js	.L191
.L193:
	mov	r15, r13
	mov	r13, QWORD PTR 16[r13]
	test	r13, r13
	je	.L291
.L195:
	mov	r14, QWORD PTR 40[r13]
	test	r14, r14
	jne	.L292
.L191:
	mov	r13, QWORD PTR 24[r13]
	test	r13, r13
	jne	.L195
.L291:
	cmp	QWORD PTR 88[rsp], r15
	je	.L190
	mov	r13, QWORD PTR 40[r15]
	test	r13, r13
	je	.L197
	mov	edx, 16
	mov	rsi, QWORD PTR 32[r15]
	mov	rdi, r12
	cmp	r13, rdx
	cmovbe	rdx, r13
	call	"memcmp"@PLT
	test	eax, eax
	jne	.L198
	mov	edx, 16
	sub	rdx, r13
	cmp	rdx, -2147483648
	jl	.L190
	mov	eax, edx
	test	edx, edx
	jne	.L198
.L197:
	movabs	rax, 4398046511105
	vmovsd	xmm2, QWORD PTR 80[rsp]
	mov	QWORD PTR 72[r15], rax
	vmovsd	QWORD PTR 64[r15], xmm2
.L214:
	mov	rax, QWORD PTR 176[rsp]
	mov	rdi, r12
	lea	rsi, 1[rax]
	call	"_ZdlPvm"@PLT
.L206:
	mov	rax, QWORD PTR 48[rsp]
	test	rax, rax
	je	.L207
	mov	rsi, rbx
	mov	rdi, rax
	call	"_ZdlPvm"@PLT
	jmp	.L207
	.p2align 6
	.p2align 3
.L290:
	sub	r14, 16
	cmp	r14, 2147483647
	jg	.L193
	mov	eax, r14d
	test	r14d, r14d
	je	.L193
	jmp	.L192
.L198:
	test	eax, eax
	jns	.L197
.L190:
	mov	rax, QWORD PTR 16[rsp]
	mov	edi, 80
	add	rax, 64
	mov	QWORD PTR 72[rsp], rax
.LEHB7:
	call	"_Znwm"@PLT
.LEHE7:
	mov	ecx, 16
	mov	QWORD PTR 32[rax], r12
	mov	rsi, r15
	lea	rdx, 32[rax]
	vpbroadcastq	xmm0, rcx
	mov	rdi, QWORD PTR 72[rsp]
	movabs	rcx, 4294967296000
	mov	QWORD PTR 64[rax], 0x000000000
	mov	QWORD PTR 72[rax], rcx
	lea	r13, 176[rsp]
	mov	r14, rax
	vmovdqu	XMMWORD PTR 40[rax], xmm0
	mov	QWORD PTR 160[rsp], r13
	mov	BYTE PTR 176[rsp], 0
	mov	QWORD PTR 168[rsp], 0
	call	"_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_N9benchmark7CounterEESt10_Select1stISA_ESt4lessIS5_ESaISA_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorISA_ERS7_"
	mov	r8, rdx
	mov	r15, rax
	test	rdx, rdx
	je	.L200
	test	rax, rax
	setne	r10b
	cmp	QWORD PTR 88[rsp], rdx
	sete	al
	or	r10b, al
	je	.L293
.L203:
	mov	rcx, QWORD PTR 88[rsp]
	movzx	edi, r10b
	mov	rdx, r8
	mov	rsi, r14
	mov	r15, r14
	call	"_ZSt29_Rb_tree_insert_and_rebalancebPSt18_Rb_tree_node_baseS0_RS_"@PLT
	mov	rax, QWORD PTR 16[rsp]
	mov	r12, QWORD PTR 160[rsp]
	add	QWORD PTR 104[rax], 1
.L205:
	vmovsd	xmm1, QWORD PTR 80[rsp]
	movabs	rax, 4398046511105
	mov	QWORD PTR 72[r15], rax
	vmovsd	QWORD PTR 64[r15], xmm1
	cmp	r12, r13
	jne	.L214
	jmp	.L206
.L181:
	mov	rdi, QWORD PTR 16[rsp]
	mov	r12, QWORD PTR 16[rdi]
.LEHB8:
	call	"_ZN9benchmark5State16StartKeepRunningEv"@PLT
	test	r12, r12
	jne	.L186
	jmp	.L182
	.p2align 6
	.p2align 3
.L187:
	sub	r12, 1
	je	.L182
.L186:
	mov	rcx, QWORD PTR 104[rsp]
	mov	rdx, QWORD PTR 96[rsp]
	mov	rdi, QWORD PTR 48[rsp]
	mov	QWORD PTR 32[rsp], rcx
	mov	rax, QWORD PTR 32[rsp]
	mov	QWORD PTR 40[rsp], rdx
	mov	rsi, rax
	mov	QWORD PTR 32[rsp], rax
	call	"_ZN12_GLOBAL__N_1L9escaped_hESt4spanIcLm18446744073709551615EESt17basic_string_viewIcSt11char_traitsIcEE.isra.0"
.LEHE8:
	test	r12, r12
	jg	.L187
	jmp	.L274
	.p2align 6
	.p2align 3
.L200:
	mov	rdi, r12
	mov	esi, 17
	mov	r12, r13
	call	"_ZdlPvm"@PLT
	mov	esi, 80
	mov	rdi, r14
	call	"_ZdlPvm"@PLT
	jmp	.L205
.L289:
	test	r13, r13
	jne	.L175
	jmp	.L281
.L222:
	mov	r15, QWORD PTR 88[rsp]
	jmp	.L190
.L221:
	mov	QWORD PTR 80[rsp], 0x000000000
	jmp	.L189
.L285:
	xor	ebx, ebx
	vzeroupper
	jmp	.L216
.L293:
	mov	r15, QWORD PTR 40[rdx]
	test	r15, r15
	je	.L203
	mov	edx, 16
	mov	rsi, QWORD PTR 32[r8]
	mov	rdi, r12
	mov	QWORD PTR 72[rsp], r8
	cmp	r15, rdx
	mov	BYTE PTR 64[rsp], r10b
	cmovbe	rdx, r15
	call	"memcmp"@PLT
	mov	r8, QWORD PTR 72[rsp]
	test	eax, eax
	je	.L294
.L204:
	shr	eax, 31
	mov	r10d, eax
	jmp	.L203
.L294:
	mov	edx, 16
	sub	rdx, r15
	cmp	rdx, -2147483648
	jl	.L223
	test	edx, edx
	movzx	r10d, BYTE PTR 64[rsp]
	mov	eax, edx
	je	.L203
	jmp	.L204
.L288:
	lea	rdi, 1[rcx]
	jmp	.L157
.L223:
	mov	r10d, 1
	jmp	.L203
.L226:
	mov	rbx, rax
	vzeroupper
	jmp	.L164
.L228:
	mov	r12, rax
	jmp	.L209
.L230:
	mov	rbx, rax
	jmp	.L151
.L232:
	mov	rbx, rax
	vzeroupper
	jmp	.L185
.L229:
	mov	rbx, rax
	jmp	.L208
.L227:
	mov	r12, rax
	vzeroupper
	jmp	.L210
.L267:
	jmp	.L268
.L269:
	jmp	.L270
	.section	.gcc_except_table,"a",@progbits
.LLSDA14964:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE14964-.LLSDACSB14964
.LLSDACSB14964:
	.uleb128 .LEHB0-.LFB14964
	.uleb128 .LEHE0-.LEHB0
	.uleb128 .L230-.LFB14964
	.uleb128 0
	.uleb128 .LEHB1-.LFB14964
	.uleb128 .LEHE1-.LEHB1
	.uleb128 .L269-.LFB14964
	.uleb128 0
	.uleb128 .LEHB2-.LFB14964
	.uleb128 .LEHE2-.LEHB2
	.uleb128 .L267-.LFB14964
	.uleb128 0
	.uleb128 .LEHB3-.LFB14964
	.uleb128 .LEHE3-.LEHB3
	.uleb128 .L226-.LFB14964
	.uleb128 0
	.uleb128 .LEHB4-.LFB14964
	.uleb128 .LEHE4-.LEHB4
	.uleb128 .L227-.LFB14964
	.uleb128 0
	.uleb128 .LEHB5-.LFB14964
	.uleb128 .LEHE5-.LEHB5
	.uleb128 .L228-.LFB14964
	.uleb128 0
	.uleb128 .LEHB6-.LFB14964
	.uleb128 .LEHE6-.LEHB6
	.uleb128 .L232-.LFB14964
	.uleb128 0
	.uleb128 .LEHB7-.LFB14964
	.uleb128 .LEHE7-.LEHB7
	.uleb128 .L229-.LFB14964
	.uleb128 0
	.uleb128 .LEHB8-.LFB14964
	.uleb128 .LEHE8-.LEHB8
	.uleb128 .L232-.LFB14964
	.uleb128 0
.LLSDACSE14964:
	.text
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.cfi_personality 0x9b,"DW.ref.__gxx_personality_v0"
	.cfi_lsda 0x1b,.LLSDAC14964
	.type	"_ZN12_GLOBAL__N_1L3armERN9benchmark5StateE.cold", @function
"_ZN12_GLOBAL__N_1L3armERN9benchmark5StateE.cold":
.LFSB14964:
.L272:
	.cfi_def_cfa 6, 16
	.cfi_offset 3, -56
	.cfi_offset 6, -16
	.cfi_offset 12, -48
	.cfi_offset 13, -40
	.cfi_offset 14, -32
	.cfi_offset 15, -24
	lea	rdi, .LC21[rip]
.LEHB9:
	call	"_ZSt20__throw_length_errorPKc"@PLT
.LEHE9:
.L225:
.L268:
	mov	rbx, rax
	vzeroupper
.L213:
	mov	rdi, r12
	call	"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv"
	mov	rdi, rbx
.LEHB10:
	call	"_Unwind_Resume"@PLT
.L231:
.L270:
	mov	rbx, rax
	vzeroupper
.L164:
	lea	rdi, 128[rsp]
	call	"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv"
.L185:
	cmp	QWORD PTR 48[rsp], 0
	je	.L212
	mov	rsi, QWORD PTR 8[rsp]
	mov	rdi, QWORD PTR 48[rsp]
	sub	rsi, rdi
	call	"_ZdlPvm"@PLT
.L212:
	lea	r12, 96[rsp]
	jmp	.L213
.L209:
	lea	rdi, 160[rsp]
	vzeroupper
	call	"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv"
.L210:
	test	r13, r13
	je	.L211
	mov	rsi, rbx
	mov	rdi, r13
	call	"_ZdlPvm"@PLT
.L211:
	mov	rbx, r12
	jmp	.L164
.L151:
	mov	rdi, r12
	vzeroupper
	call	"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv"
	mov	rdi, rbx
	call	"_Unwind_Resume"@PLT
.LEHE10:
.L271:
	lea	rcx, .LC17[rip]
	mov	edx, 990
	lea	rsi, .LC18[rip]
	lea	rdi, .LC19[rip]
	vzeroupper
	call	"__assert_fail"@PLT
.L273:
	lea	rdi, .LC15[rip]
.LEHB11:
	call	"_ZSt20__throw_length_errorPKc"@PLT
.LEHE11:
.L208:
	lea	rdi, 160[rsp]
	vzeroupper
	call	"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv"
	jmp	.L185
.L274:
	lea	rcx, .LC22[rip]
	mov	edx, 1134
	lea	rsi, .LC18[rip]
	lea	rdi, .LC23[rip]
	call	"__assert_fail"@PLT
	.cfi_endproc
.LFE14964:
	.section	.gcc_except_table
.LLSDAC14964:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSEC14964-.LLSDACSBC14964
.LLSDACSBC14964:
	.uleb128 .LEHB9-.LCOLDB28
	.uleb128 .LEHE9-.LEHB9
	.uleb128 .L225-.LCOLDB28
	.uleb128 0
	.uleb128 .LEHB10-.LCOLDB28
	.uleb128 .LEHE10-.LEHB10
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB11-.LCOLDB28
	.uleb128 .LEHE11-.LEHB11
	.uleb128 .L231-.LCOLDB28
	.uleb128 0
.LLSDACSEC14964:
	.section	.text.unlikely
	.text
	.size	"_ZN12_GLOBAL__N_1L3armERN9benchmark5StateE", .-"_ZN12_GLOBAL__N_1L3armERN9benchmark5StateE"
	.section	.text.unlikely
	.size	"_ZN12_GLOBAL__N_1L3armERN9benchmark5StateE.cold", .-"_ZN12_GLOBAL__N_1L3armERN9benchmark5StateE.cold"
.LCOLDE28:
	.text
.LHOTE28:
	.section	.text.unlikely
.LCOLDB32:
	.section	.text.startup
.LHOTB32:
	.p2align 6
	.type	"_GLOBAL__sub_I_main", @function
"_GLOBAL__sub_I_main":
.LFB16387:
	.cfi_startproc
	.cfi_personality 0x9b,"DW.ref.__gxx_personality_v0"
	.cfi_lsda 0x1b,.LLSDA16387
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	push	r13
	push	r12
	push	rbx
	sub	rsp, 168
	.cfi_offset 13, -24
	.cfi_offset 12, -32
	.cfi_offset 3, -40
.LEHB12:
	call	"_ZN9benchmark8internal17InitializeStreamsEv"@PLT
	mov	edi, 232
	lea	r12, 144[rsp]
	call	"_Znwm"@PLT
.LEHE12:
	vpxor	xmm0, xmm0, xmm0
	lea	rsi, 128[rsp]
	vmovdqu	YMMWORD PTR 128[rsp], ymm0
	mov	rbx, rax
	mov	eax, 29281
	mov	QWORD PTR 128[rsp], r12
	mov	rdi, rbx
	mov	WORD PTR 144[rsp], ax
	mov	BYTE PTR 146[rsp], 109
	mov	QWORD PTR 136[rsp], 3
	vzeroupper
.LEHB13:
	call	"_ZN9benchmark8internal9BenchmarkC2ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE"@PLT
.LEHE13:
	lea	rax, "_ZTVN9benchmark8internal17FunctionBenchmarkE"[rip+16]
	mov	rdi, rbx
	mov	QWORD PTR [rbx], rax
	lea	rax, "_ZN12_GLOBAL__N_1L3armERN9benchmark5StateE"[rip]
	mov	QWORD PTR 224[rbx], rax
.LEHB14:
	call	"_ZN9benchmark8internal25RegisterBenchmarkInternalEPNS0_9BenchmarkE"@PLT
.LEHE14:
	vpxor	xmm0, xmm0, xmm0
	mov	edi, 16
	mov	r13, rax
	mov	QWORD PTR 112[rsp], 0
	vmovdqa	XMMWORD PTR 96[rsp], xmm0
.LEHB15:
	call	"_Znwm"@PLT
.LEHE15:
	mov	QWORD PTR [rax], 64
	lea	rdx, 16[rax]
	lea	rsi, 96[rsp]
	mov	rdi, r13
	mov	QWORD PTR 8[rax], 0
	mov	QWORD PTR 96[rsp], rax
	mov	QWORD PTR 112[rsp], rdx
	mov	QWORD PTR 104[rsp], rdx
.LEHB16:
	call	"_ZN9benchmark8internal9Benchmark4ArgsERKSt6vectorIlSaIlEE"@PLT
.LEHE16:
	vpxor	xmm0, xmm0, xmm0
	mov	edi, 16
	mov	r13, rax
	mov	QWORD PTR 80[rsp], 0
	vmovdqa	XMMWORD PTR 64[rsp], xmm0
.LEHB17:
	call	"_Znwm"@PLT
.LEHE17:
	lea	rdx, 16[rax]
	lea	rsi, 64[rsp]
	mov	rdi, r13
	mov	QWORD PTR 64[rsp], rax
	vmovdqa	xmm0, XMMWORD PTR .LC29[rip]
	mov	QWORD PTR 80[rsp], rdx
	mov	QWORD PTR 72[rsp], rdx
	vmovdqu	XMMWORD PTR [rax], xmm0
.LEHB18:
	call	"_ZN9benchmark8internal9Benchmark4ArgsERKSt6vectorIlSaIlEE"@PLT
.LEHE18:
	vpxor	xmm0, xmm0, xmm0
	mov	edi, 16
	mov	r13, rax
	mov	QWORD PTR 48[rsp], 0
	vmovdqa	XMMWORD PTR 32[rsp], xmm0
.LEHB19:
	call	"_Znwm"@PLT
.LEHE19:
	mov	QWORD PTR [rax], 1024
	lea	rdx, 16[rax]
	lea	rsi, 32[rsp]
	mov	rdi, r13
	mov	QWORD PTR 8[rax], 0
	mov	QWORD PTR 32[rsp], rax
	mov	QWORD PTR 48[rsp], rdx
	mov	QWORD PTR 40[rsp], rdx
.LEHB20:
	call	"_ZN9benchmark8internal9Benchmark4ArgsERKSt6vectorIlSaIlEE"@PLT
.LEHE20:
	vpxor	xmm0, xmm0, xmm0
	mov	edi, 16
	mov	r13, rax
	mov	QWORD PTR 16[rsp], 0
	vmovdqa	XMMWORD PTR [rsp], xmm0
.LEHB21:
	call	"_Znwm"@PLT
.LEHE21:
	lea	rdx, 16[rax]
	mov	rsi, rsp
	mov	rdi, r13
	mov	QWORD PTR [rsp], rax
	vmovdqa	xmm0, XMMWORD PTR .LC30[rip]
	mov	QWORD PTR 16[rsp], rdx
	mov	QWORD PTR 8[rsp], rdx
	vmovdqu	XMMWORD PTR [rax], xmm0
.LEHB22:
	call	"_ZN9benchmark8internal9Benchmark4ArgsERKSt6vectorIlSaIlEE"@PLT
.LEHE22:
	mov	rdi, QWORD PTR [rsp]
	test	rdi, rdi
	je	.L309
	mov	rsi, QWORD PTR 16[rsp]
	sub	rsi, rdi
	call	"_ZdlPvm"@PLT
.L309:
	mov	rdi, QWORD PTR 32[rsp]
	test	rdi, rdi
	je	.L310
	mov	rsi, QWORD PTR 48[rsp]
	sub	rsi, rdi
	call	"_ZdlPvm"@PLT
.L310:
	mov	rdi, QWORD PTR 64[rsp]
	test	rdi, rdi
	je	.L311
	mov	rsi, QWORD PTR 80[rsp]
	sub	rsi, rdi
	call	"_ZdlPvm"@PLT
.L311:
	mov	rdi, QWORD PTR 96[rsp]
	test	rdi, rdi
	je	.L312
	mov	rsi, QWORD PTR 112[rsp]
	sub	rsi, rdi
	call	"_ZdlPvm"@PLT
.L312:
	mov	rdi, QWORD PTR 128[rsp]
	cmp	rdi, r12
	je	.L346
	mov	rax, QWORD PTR 144[rsp]
	lea	rsi, 1[rax]
	call	"_ZdlPvm"@PLT
.L346:
	add	rsp, 168
	pop	rbx
	pop	r12
	pop	r13
	pop	rbp
	.cfi_remember_state
	.cfi_def_cfa 7, 8
	ret
.L317:
	.cfi_restore_state
	mov	r12, rax
	jmp	.L315
.L321:
	mov	r12, rax
	jmp	.L314
.L325:
	mov	r12, rax
	jmp	.L307
.L320:
	mov	r12, rax
	vzeroupper
	jmp	.L308
.L324:
	mov	r12, rax
	jmp	.L304
.L319:
	mov	r12, rax
	vzeroupper
	jmp	.L305
.L323:
	mov	r12, rax
	jmp	.L301
.L318:
	mov	r12, rax
	vzeroupper
	jmp	.L302
.L322:
	mov	r12, rax
	jmp	.L297
.L326:
	mov	r12, rax
	vzeroupper
	jmp	.L298
	.section	.gcc_except_table
.LLSDA16387:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16387-.LLSDACSB16387
.LLSDACSB16387:
	.uleb128 .LEHB12-.LFB16387
	.uleb128 .LEHE12-.LEHB12
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB13-.LFB16387
	.uleb128 .LEHE13-.LEHB13
	.uleb128 .L317-.LFB16387
	.uleb128 0
	.uleb128 .LEHB14-.LFB16387
	.uleb128 .LEHE14-.LEHB14
	.uleb128 .L326-.LFB16387
	.uleb128 0
	.uleb128 .LEHB15-.LFB16387
	.uleb128 .LEHE15-.LEHB15
	.uleb128 .L322-.LFB16387
	.uleb128 0
	.uleb128 .LEHB16-.LFB16387
	.uleb128 .LEHE16-.LEHB16
	.uleb128 .L318-.LFB16387
	.uleb128 0
	.uleb128 .LEHB17-.LFB16387
	.uleb128 .LEHE17-.LEHB17
	.uleb128 .L323-.LFB16387
	.uleb128 0
	.uleb128 .LEHB18-.LFB16387
	.uleb128 .LEHE18-.LEHB18
	.uleb128 .L319-.LFB16387
	.uleb128 0
	.uleb128 .LEHB19-.LFB16387
	.uleb128 .LEHE19-.LEHB19
	.uleb128 .L324-.LFB16387
	.uleb128 0
	.uleb128 .LEHB20-.LFB16387
	.uleb128 .LEHE20-.LEHB20
	.uleb128 .L320-.LFB16387
	.uleb128 0
	.uleb128 .LEHB21-.LFB16387
	.uleb128 .LEHE21-.LEHB21
	.uleb128 .L325-.LFB16387
	.uleb128 0
	.uleb128 .LEHB22-.LFB16387
	.uleb128 .LEHE22-.LEHB22
	.uleb128 .L321-.LFB16387
	.uleb128 0
.LLSDACSE16387:
	.section	.text.startup
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.cfi_personality 0x9b,"DW.ref.__gxx_personality_v0"
	.cfi_lsda 0x1b,.LLSDAC16387
	.type	"_GLOBAL__sub_I_main.cold", @function
"_GLOBAL__sub_I_main.cold":
.LFSB16387:
.L315:
	.cfi_def_cfa 6, 16
	.cfi_offset 3, -40
	.cfi_offset 6, -16
	.cfi_offset 12, -32
	.cfi_offset 13, -24
	mov	r13d, 1
	vzeroupper
.L299:
	lea	rdi, 128[rsp]
	call	"_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv"
	test	r13b, r13b
	je	.L316
	mov	esi, 232
	mov	rdi, rbx
	call	"_ZdlPvm"@PLT
.L316:
	mov	rdi, r12
.LEHB23:
	call	"_Unwind_Resume"@PLT
.LEHE23:
.L314:
	mov	rdi, rsp
	vzeroupper
	call	"_ZNSt12_Vector_baseIlSaIlEED2Ev"
.L308:
	lea	rdi, 32[rsp]
	call	"_ZNSt12_Vector_baseIlSaIlEED2Ev"
.L305:
	lea	rdi, 64[rsp]
	call	"_ZNSt12_Vector_baseIlSaIlEED2Ev"
.L302:
	lea	rdi, 96[rsp]
	call	"_ZNSt12_Vector_baseIlSaIlEED2Ev"
.L298:
	xor	r13d, r13d
	jmp	.L299
.L307:
	mov	rdi, rsp
	vzeroupper
	call	"_ZNSt12_Vector_baseIlSaIlEED2Ev"
	jmp	.L308
.L304:
	lea	rdi, 32[rsp]
	vzeroupper
	call	"_ZNSt12_Vector_baseIlSaIlEED2Ev"
	jmp	.L305
.L301:
	lea	rdi, 64[rsp]
	vzeroupper
	call	"_ZNSt12_Vector_baseIlSaIlEED2Ev"
	jmp	.L302
.L297:
	lea	rdi, 96[rsp]
	vzeroupper
	call	"_ZNSt12_Vector_baseIlSaIlEED2Ev"
	jmp	.L298
	.cfi_endproc
.LFE16387:
	.section	.gcc_except_table
.LLSDAC16387:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSEC16387-.LLSDACSBC16387
.LLSDACSBC16387:
	.uleb128 .LEHB23-.LCOLDB32
	.uleb128 .LEHE23-.LEHB23
	.uleb128 0
	.uleb128 0
.LLSDACSEC16387:
	.section	.text.unlikely
	.section	.text.startup
	.size	"_GLOBAL__sub_I_main", .-"_GLOBAL__sub_I_main"
	.section	.text.unlikely
	.size	"_GLOBAL__sub_I_main.cold", .-"_GLOBAL__sub_I_main.cold"
.LCOLDE32:
	.section	.text.startup
.LHOTE32:
	.section	.init_array,"aw"
	.align 8
	.quad	"_GLOBAL__sub_I_main"
	.weak	"_ZN8mustache9kEntitiesE"
	.section	.rodata._ZN8mustache9kEntitiesE,"aG",@progbits,_ZN8mustache9kEntitiesE,comdat
	.align 32
	.type	"_ZN8mustache9kEntitiesE", @gnu_unique_object
	.size	"_ZN8mustache9kEntitiesE", 2048
"_ZN8mustache9kEntitiesE":
	.quad	72057594037927936
	.quad	72057594037927937
	.quad	72057594037927938
	.quad	72057594037927939
	.quad	72057594037927940
	.quad	72057594037927941
	.quad	72057594037927942
	.quad	72057594037927943
	.quad	72057594037927944
	.quad	72057594037927945
	.quad	72057594037927946
	.quad	72057594037927947
	.quad	72057594037927948
	.quad	72057594037927949
	.quad	72057594037927950
	.quad	72057594037927951
	.quad	72057594037927952
	.quad	72057594037927953
	.quad	72057594037927954
	.quad	72057594037927955
	.quad	72057594037927956
	.quad	72057594037927957
	.quad	72057594037927958
	.quad	72057594037927959
	.quad	72057594037927960
	.quad	72057594037927961
	.quad	72057594037927962
	.quad	72057594037927963
	.quad	72057594037927964
	.quad	72057594037927965
	.quad	72057594037927966
	.quad	72057594037927967
	.quad	72057594037927968
	.quad	72057594037927969
	.quad	432410935499780390
	.quad	72057594037927971
	.quad	72057594037927972
	.quad	72057594037927973
	.quad	360288225478926630
	.quad	360288224552362790
	.quad	72057594037927976
	.quad	72057594037927977
	.quad	72057594037927978
	.quad	72057594037927979
	.quad	72057594037927980
	.quad	72057594037927981
	.quad	72057594037927982
	.quad	72057594037927983
	.quad	72057594037927984
	.quad	72057594037927985
	.quad	72057594037927986
	.quad	72057594037927987
	.quad	72057594037927988
	.quad	72057594037927989
	.quad	72057594037927990
	.quad	72057594037927991
	.quad	72057594037927992
	.quad	72057594037927993
	.quad	72057594037927994
	.quad	72057594037927995
	.quad	288230377149197350
	.quad	72057594037927997
	.quad	288230377149196070
	.quad	72057594037927999
	.quad	72057594037928000
	.quad	72057594037928001
	.quad	72057594037928002
	.quad	72057594037928003
	.quad	72057594037928004
	.quad	72057594037928005
	.quad	72057594037928006
	.quad	72057594037928007
	.quad	72057594037928008
	.quad	72057594037928009
	.quad	72057594037928010
	.quad	72057594037928011
	.quad	72057594037928012
	.quad	72057594037928013
	.quad	72057594037928014
	.quad	72057594037928015
	.quad	72057594037928016
	.quad	72057594037928017
	.quad	72057594037928018
	.quad	72057594037928019
	.quad	72057594037928020
	.quad	72057594037928021
	.quad	72057594037928022
	.quad	72057594037928023
	.quad	72057594037928024
	.quad	72057594037928025
	.quad	72057594037928026
	.quad	72057594037928027
	.quad	72057594037928028
	.quad	72057594037928029
	.quad	72057594037928030
	.quad	72057594037928031
	.quad	72057594037928032
	.quad	72057594037928033
	.quad	72057594037928034
	.quad	72057594037928035
	.quad	72057594037928036
	.quad	72057594037928037
	.quad	72057594037928038
	.quad	72057594037928039
	.quad	72057594037928040
	.quad	72057594037928041
	.quad	72057594037928042
	.quad	72057594037928043
	.quad	72057594037928044
	.quad	72057594037928045
	.quad	72057594037928046
	.quad	72057594037928047
	.quad	72057594037928048
	.quad	72057594037928049
	.quad	72057594037928050
	.quad	72057594037928051
	.quad	72057594037928052
	.quad	72057594037928053
	.quad	72057594037928054
	.quad	72057594037928055
	.quad	72057594037928056
	.quad	72057594037928057
	.quad	72057594037928058
	.quad	72057594037928059
	.quad	72057594037928060
	.quad	72057594037928061
	.quad	72057594037928062
	.quad	72057594037928063
	.quad	72057594037928064
	.quad	72057594037928065
	.quad	72057594037928066
	.quad	72057594037928067
	.quad	72057594037928068
	.quad	72057594037928069
	.quad	72057594037928070
	.quad	72057594037928071
	.quad	72057594037928072
	.quad	72057594037928073
	.quad	72057594037928074
	.quad	72057594037928075
	.quad	72057594037928076
	.quad	72057594037928077
	.quad	72057594037928078
	.quad	72057594037928079
	.quad	72057594037928080
	.quad	72057594037928081
	.quad	72057594037928082
	.quad	72057594037928083
	.quad	72057594037928084
	.quad	72057594037928085
	.quad	72057594037928086
	.quad	72057594037928087
	.quad	72057594037928088
	.quad	72057594037928089
	.quad	72057594037928090
	.quad	72057594037928091
	.quad	72057594037928092
	.quad	72057594037928093
	.quad	72057594037928094
	.quad	72057594037928095
	.quad	72057594037928096
	.quad	72057594037928097
	.quad	72057594037928098
	.quad	72057594037928099
	.quad	72057594037928100
	.quad	72057594037928101
	.quad	72057594037928102
	.quad	72057594037928103
	.quad	72057594037928104
	.quad	72057594037928105
	.quad	72057594037928106
	.quad	72057594037928107
	.quad	72057594037928108
	.quad	72057594037928109
	.quad	72057594037928110
	.quad	72057594037928111
	.quad	72057594037928112
	.quad	72057594037928113
	.quad	72057594037928114
	.quad	72057594037928115
	.quad	72057594037928116
	.quad	72057594037928117
	.quad	72057594037928118
	.quad	72057594037928119
	.quad	72057594037928120
	.quad	72057594037928121
	.quad	72057594037928122
	.quad	72057594037928123
	.quad	72057594037928124
	.quad	72057594037928125
	.quad	72057594037928126
	.quad	72057594037928127
	.quad	72057594037928128
	.quad	72057594037928129
	.quad	72057594037928130
	.quad	72057594037928131
	.quad	72057594037928132
	.quad	72057594037928133
	.quad	72057594037928134
	.quad	72057594037928135
	.quad	72057594037928136
	.quad	72057594037928137
	.quad	72057594037928138
	.quad	72057594037928139
	.quad	72057594037928140
	.quad	72057594037928141
	.quad	72057594037928142
	.quad	72057594037928143
	.quad	72057594037928144
	.quad	72057594037928145
	.quad	72057594037928146
	.quad	72057594037928147
	.quad	72057594037928148
	.quad	72057594037928149
	.quad	72057594037928150
	.quad	72057594037928151
	.quad	72057594037928152
	.quad	72057594037928153
	.quad	72057594037928154
	.quad	72057594037928155
	.quad	72057594037928156
	.quad	72057594037928157
	.quad	72057594037928158
	.quad	72057594037928159
	.quad	72057594037928160
	.quad	72057594037928161
	.quad	72057594037928162
	.quad	72057594037928163
	.quad	72057594037928164
	.quad	72057594037928165
	.quad	72057594037928166
	.quad	72057594037928167
	.quad	72057594037928168
	.quad	72057594037928169
	.quad	72057594037928170
	.quad	72057594037928171
	.quad	72057594037928172
	.quad	72057594037928173
	.quad	72057594037928174
	.quad	72057594037928175
	.quad	72057594037928176
	.quad	72057594037928177
	.quad	72057594037928178
	.quad	72057594037928179
	.quad	72057594037928180
	.quad	72057594037928181
	.quad	72057594037928182
	.quad	72057594037928183
	.quad	72057594037928184
	.quad	72057594037928185
	.quad	72057594037928186
	.quad	72057594037928187
	.quad	72057594037928188
	.quad	72057594037928189
	.quad	72057594037928190
	.quad	72057594037928191
	.section	.rodata.cst16,"aM",@progbits,16
	.align 16
.LC25:
	.quad	2336649543156066420
	.quad	8389960310810110311
	.align 16
.LC26:
	.quad	7526763062408607337
	.quad	8315180351695647333
	.align 16
.LC27:
	.quad	7309447144450324834
	.quad	7236844123925471090
	.align 16
.LC29:
	.quad	64
	.quad	5
	.align 16
.LC30:
	.quad	1024
	.quad	5
	.hidden	"DW.ref.__gxx_personality_v0"
	.weak	"DW.ref.__gxx_personality_v0"
	.section	.data.rel.local.DW.ref.__gxx_personality_v0,"awG",@progbits,DW.ref.__gxx_personality_v0,comdat
	.align 8
	.type	"DW.ref.__gxx_personality_v0", @object
	.size	"DW.ref.__gxx_personality_v0", 8
"DW.ref.__gxx_personality_v0":
	.quad	"__gxx_personality_v0"
	.globl	"__gxx_personality_v0"
	.ident	"GCC: (Debian 16.2.0-4) 16.2.0"
	.section	.note.GNU-stack,"",@progbits
