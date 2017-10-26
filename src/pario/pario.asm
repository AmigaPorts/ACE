; pario.asm
; Parallel Port I/O Routines for C Programs
; v1.5
; Tom Handley
; 17 Jun 93
; Based on AmigaMail Jun 92, 4play/read34.asm
; Assembled with CAPE v2.51

	SECTION TEXT

	xdef	Name		; Name of our application
Name	dc.b	'pario',0

; function names for linker

	xdef	_getport        ; Get   parallel port access
	xdef	_freeport       ; Free  parallel port access
        xdef    _portdir        ; Set   parallel port data direction
        xdef    _portddr        ; Set   parallel port data direction bits
        xdef    _rdport         ; Read  parallel port data
        xdef    _wrport         ; Write parallel port data
        xdef    _busydir        ; Set   BUSY control line data direction
        xdef    _rdbusy         ; Read  BUSY control line state
        xdef    _setbusy        ; Set   BUSY control line state
        xdef    _poutdir        ; Set   POUT control line data direction
        xdef    _rdpout         ; Read  POUT control line state
        xdef    _setpout        ; Set   POUT control line state
        xdef    _seldir         ; Set   SEL  control line data direction
        xdef    _rdsel          ; Read  SEL  control line state
        xdef    _setsel         ; Set   SEL  control line state

	xref	_SysBase	; exec system base (from c.o)

	INCLUDE	"resources/misc.i"

	xdef	MiscName
MiscName	MISCNAME	; macro from resources/misc.i

	xdef	_MiscResource
_MiscResource	dc.l	0	; place to store misc.resource base

; CIA parallel port hardware addresses (from amiga.lib)

	xref	_ciaaprb	; CIAA port B
	xref	_ciaaddrb	; CIAA Port B data direction register
	xref	_ciabpra	; CIAB Port A control lines (BUSY, POUT, SEL)
	xref	_ciabddra	; CIAB Port A data direction register

; from amiga.lib

	xref	_LVOOpenResource
	xref	_LVOAllocMiscResource
	xref	_LVOFreeMiscResource

_getport
;
; This routine requests exclusive access to the parallel port in a system
; friendly way and sets up the data port and control BUSY, POUT, and SEL lines
; to inputs.  Affects all CIA-A port B data direction register (CIAA DDRB) bits
; and CIA-B port A data direction register (CIAB DDRA) bits 0-2.
;
; Exit:
;       d0 =  0 if port access successful
;            20 if unable to open misc.resource
;            30 if unable to allocate port
;            40 if unable to allocate control bits (BUSY, POUT, SEL)
;
; Example:
;       extern LONG getport(void);
;       extern void freeport(void);
;       LONG   port_error;
;
;       if(port_error = getport())
;          /* Abort */
;
;       /* Do something with the port */
;
;       if(port_error == 0)
;          freeport();
;       exit(port_error);
;
; See Also:
;       _freeport
;
	movem.l	a2-a6/d2-d7,-(sp)	; save registers on the stack

; Open the misc.resource

	lea	MiscName,a1		; put name of misc.resource in a1
	movea.l	_SysBase,a6		; put SysBase in a6
	jsr	_LVOOpenResource(a6)
	move.l	d0,_MiscResource	; store address of misc.resource
	bne.s	grabit

; Couldn't open misc.resource. Exit with error

	moveq	#20,d0			; put error code in d0
	bra.s	done

; This is where we grab the hardware.  If some other task has allocated
; the parallel data port or the parallel control bits, this routine will
; return non-zero.

; This part grabs the port itself

grabit	lea	Name,a1			; The name of our app
	moveq	#MR_PARALLELPORT,d0	; We want the parallel port
	movea.l	_MiscResource,a6	; MiscResource Base is in A6
	jsr	_LVOAllocMiscResource(a6)
	move.l	d0,d1
	beq.s	grab2

; Someone has the port. Exit with error

	moveq	#30,d0			; put error code in d0
	bra.s	done

; This part grabs the control bits (BUSY, POUT, and SEL.)

grab2	lea	Name,a1			; The name of our app
	moveq	#MR_PARALLELBITS,d0	; We want the port control bits
	jsr	_LVOAllocMiscResource(a6)
	move.l	d0,d1
	beq.s	setread

; Someone has the port control bits. Free the port and exit with error

	moveq	#40,d2
	bra.s	freepar

; Set up parallel port for reading

setread	move.b	#$00,_ciaaddrb		; set all data port lines to input
	andi.b	#$F8,_ciabddra		; set BUSY, POUT, and SEL lines to input

; We now have exclusive access to the port. Now we restore the registers and
; return to the caller

	bra.s	done

; Something happened after we had exculsive access to the port.  Free the port
; and exit with error

freepar	moveq	#MR_PARALLELPORT,d0
	movea.l	_MiscResource,a6
	jsr	_LVOFreeMiscResource(a6)

	move.l	d2,d0			; put error code in d0

; Restore registers and return
; (error code is in d0)

done	movem.l (sp)+,a2-a6/d2-d7	; pop regs
	rts

_freeport
;
; This routine releases exclusive access to the parallel port and control
; lines.
;
; Warning:
;       Don't call this routine if you got an error from getport() as some of
;       the resources might not have been opened, etc.
;
; Example:
;       extern LONG getport(void);
;       extern void freeport(void);
;       LONG   port_error;
;
;       if(port_error = getport())
;          /* Abort */
;
;       /* Do something with the port */
;
;       if(port_error == 0)
;          freeport();
;       exit(port_error);
;
; See Also:
;       _getport
;
	movem.l	a2-a6/d2-d7,-(sp)	; save registers on the stack

; free control lines

	moveq	#MR_PARALLELBITS,d0
	movea.l	_MiscResource,a6
	jsr	_LVOFreeMiscResource(a6)

; free parallel port

	moveq	#MR_PARALLELPORT,d0
	movea.l	_MiscResource,a6
	jsr	_LVOFreeMiscResource(a6)

; Clean up, restore registers, and return

	movem.l (sp)+,a2-a6/d2-d7	; pop regs
	rts

*******************************************************************************
*                                                                             *
*     The following routines allow us to access and manipulate the parallel   *
*  port from a C program. This assumes we `survived' the above and actually   *
*  own the port ;-)                                                           *
*                                                                             *
*     d0 is used as a scratch register. All other registers are preserved.    *
*                                                                             *
*******************************************************************************

_portdir
;
; This routine sets the port data direction to input or output.  All port data
; lines are set to the same direction.  Affects all CIA-A port B data direction
; register (CIAA DDRB) bits.
;
; Entry:
;       4(sp) = direction (0 = input, 1 = output)
;
; Example:
;       extern void portdir(UBYTE);
;       UBYTE  direction;
;
;       direction = 0;
;       portdir(direction);     /* Set port data lines to input*/
;
; See Also:
;       _portddr, _rdport, _wrport
;
	tst.b	4(sp)                   ; input or output?
        bne.s   1$
        move.b  #$00,_ciaaddrb          ; set port dir to input
        bra.s   2$
1$      move.b  #$FF,_ciaaddrb          ; set port dir to output
2$      rts

_portddr
;
; This routine sets the individual port data direction bits to input or
; output.  Affects all CIA-A port B data direction register (CIAA DDRB) bits.
;
; Entry:
;       4(sp) = direction bit mask (bit = 0 for input, 1 for output)
;
; Example:
;       extern void portddr(UBYTE);
;       UBYTE  bitmask;
;
;       bitmask = 0xF0;
;       portddr(bitmask);  /* Set bits 0-3 as inputs, bits 4-7 as outputs */
;
; See Also:
;       _portdir, _rdport, _wrport
;
        move.b  4(sp),_ciaaddrb         ; set data direction bits
        rts

_rdport
;
; This routine reads data from the parallel port.
;
; Exit:
;       d0 = port data
;
; Example:
;       extern UBYTE rdport(void);
;       UBYTE  portdata;
;
;       portdata = rdport();   /* Read port data */
;
; See Also:
;       _portdir, _portddr, _wrport
;
	move.b	_ciaaprb,d0             ; put port data in d0
	rts

_wrport
;
; This routine writes data to the parallel port.  Affects all CIA-A port B data
; register (CIAA PRB) bits.
;
; Entry:
;       4(sp) = port data
;
; Example:
;       extern void wrport(UBYTE);
;       UBYTE  portdata;
;
;       wrport(portdata);   /* Write port data */
;
; See Also:
;       _portdir, _portddr, _rdport
;
        move.b  4(sp),_ciaaprb          ; write port data
        rts

_busydir
;
; This routine sets the BUSY data direction to input or output.  Affects CIA-B
; port A data direction register (CIAB DDRA) bit 0.
;
; Entry:
;       4(sp) = direction (0 = input, 1 = output)
;
; Example:
;       extern void busydir(UBYTE);
;       UBYTE  direction;
;
;       direction = 0;
;       busydir(direction);  /* Set BUSY control line to input */
;
; See Also:
;       _rdbusy, _setbusy
;
	tst.b	4(sp)                   ; input or output?
        bne.s   1$
        andi.b  #%11111110,_ciabddra    ; set BUSY dir to input
        bra.s   2$
1$      ori.b   #%00000001,_ciabddra    ; set BUSY dir to output
2$      rts

_rdbusy
;
; This routine reads the state of the BUSY line.
;
; Exit:
;       d0 = 0 for low level, 1 for high level
;
; Example:
;       extern UBYTE rdbusy(void);
;       UBYTE  statedata;
;
;       statedata = rdbusy();  /* Read state of BUSY line */
;
; See Also:
;       _busydir, _setbusy
;
	move.b	_ciabpra,d0             ; put BUSY state in d0
        andi.b  #%00000001,d0           ; mask BUSY bit
	rts

_setbusy
;
; This function sets the BUSY line to a low or high level.  Affects CIA-B port A
; data register (CIAB PRA) bit 0.
;
; Entry:
;       4(sp) = 0 for low level, 1 for high level
;
; Example:
;       extern void setbusy(UBYTE);
;       UBYTE  statedata;
;
;       statedata = 0;
;       setbusy(statedata);  /* Set BUSY line to a low level */
;
; See Also:
;       _busydir, _rdbusy
;
	tst.b	4(sp)                   ; 0 or 1?
        bne.s   1$
        andi.b  #%11111110,_ciabpra     ; set BUSY to 0
        bra.s   2$
1$      ori.b   #%00000001,_ciabpra     ; set BUSY to 1
2$      rts

_poutdir
;
; This routine sets the POUT data direction to input or output.  Affects CIA-B
; port A data direction register (CIAB DDRA) bit 1.
;
; Entry:
;       4(sp) = direction (0 = input, 1 = output)
;
; Example:
;       extern void poutdir(UBYTE);
;       UBYTE  direction;
;
;       direction = 0;
;       poutdir(direction);  /* Set POUT control line to input */
;
; See Also:
;       _rdpout, _setpout
;
	tst.b	4(sp)                   ; input or output?
        bne.s   1$
        andi.b  #%11111101,_ciabddra    ; set POUT dir to input
        bra.s   2$
1$      ori.b   #%00000010,_ciabddra    ; set POUT dir to output
2$      rts

_rdpout
;
; This routine reads the state of the POUT line.
;
; Exit:
;       d0 = 0 for low level, 1 for high level
;
; Example:
;       extern UBYTE rdpout(void);
;       UBYTE  statedata;
;
;       statedata = rdpout();  /* Read state of POUT line */
;
; See Also:
;       _poutdir, _setpout
;
	move.b	_ciabpra,d0             ; put POUT state in d0
        andi.b  #%00000010,d0           ; mask POUT bit
        lsr.b   #1,d0                   ; shift bit to LSB
	rts

_setpout
;
; This function sets the POUT line to a low or high level.  Affects CIA-B port A
; data register (CIAB PRA) bit 1.
;
; Entry:
;       4(sp) = 0 for low level, 1 for high level
;
; Example:
;       extern void setpout(UBYTE);
;       UBYTE  statedata;
;
;       statedata = 0;
;       setpout(statedata);  /* Set POUT line to a low level */
;
; See Also:
;       _poutdir, _rdpout
;
	tst.b	4(sp)                   ; 0 or 1?
        bne.s   1$
        andi.b  #%11111101,_ciabpra     ; set POUT to 0
        bra.s   2$
1$      ori.b   #%00000010,_ciabpra     ; set POUT to 1
2$      rts

_seldir
;
; This routine sets the SEL data direction to input or output.  Affects CIA-B
; port A data direction register (CIAB DDRA) bit 2.
;
; Entry:
;       4(sp) = direction (0 = input, 1 = output)
;
; Example:
;       extern void seldir(UBYTE);
;       UBYTE  direction;
;
;       direction = 0;
;       seldir(direction);  /* Set SEL control line to input */
;
; See Also:
;       _rdsel, _setsel
;
	tst.b	4(sp)                   ; input or output?
        bne.s   1$
        andi.b  #%11111011,_ciabddra    ; set SEL dir to input
        bra.s   2$
1$      ori.b   #%00000100,_ciabddra    ; set SEL dir to output
2$      rts

_rdsel
;
; This routine reads the state of the SEL line.
;
; Exit:
;       d0 = 0 for low level, 1 for high level
;
; Example:
;       extern UBYTE rdsel(void);
;       UBYTE  statedata;
;
;       statedata = rdsel();  /* Read state of SEL line */
;
; See Also:
;       _seldir, _setsel
;
	move.b	_ciabpra,d0             ; put SEL state in d0
        andi.b  #%00000100,d0           ; mask SEL bit
        lsr.b   #2,d0                   ; shift bit to LSB
	rts

_setsel
;
; This function sets the SEL line to a low or high level.  Affects CIA-B port A
; data register (CIAB PRA) bit 2.
;
; Entry:
;       4(sp) = 0 for low level, 1 for high level
;
; Example:
;       extern void setsel(UBYTE);
;       UBYTE  statedata;
;
;       statedata = 0;
;       setsel(statedata);  /* Set SEL line to a low level */
;
; See Also:
;       _seldir, _rdsel
;
	tst.b	4(sp)                   ; 0 or 1?
        bne.s   1$
        andi.b  #%11111011,_ciabpra     ; set SEL to 0
        bra.s   2$
1$      ori.b   #%00000100,_ciabpra     ; set SEL to 1
2$      rts

	end

