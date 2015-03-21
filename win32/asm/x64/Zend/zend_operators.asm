
const segment
tolower_map db	00h
	db	01h
	db	02h
	db	03h
	db	04h
	db	05h
	db	06h
	db	07h
	db	08h
	db	09h
	db	0ah
	db	0bh
	db	0ch
	db	0dh
	db	0eh
	db	0fh
	db	010h
	db	011h
	db	012h
	db	013h
	db	014h
	db	015h
	db	016h
	db	017h
	db	018h
	db	019h
	db	01ah
	db	01bh
	db	01ch
	db	01dh
	db	01eh
	db	01fh
	db	020h
	db	021h
	db	022h
	db	023h
	db	024h
	db	025h
	db	026h
	db	027h
	db	028h
	db	029h
	db	02ah
	db	02bh
	db	02ch
	db	02dh
	db	02eh
	db	02fh
	db	030h
	db	031h
	db	032h
	db	033h
	db	034h
	db	035h
	db	036h
	db	037h
	db	038h
	db	039h
	db	03ah
	db	03bh
	db	03ch
	db	03dh
	db	03eh
	db	03fh
	db	040h
	db	061h
	db	062h
	db	063h
	db	064h
	db	065h
	db	066h
	db	067h
	db	068h
	db	069h
	db	06ah
	db	06bh
	db	06ch
	db	06dh
	db	06eh
	db	06fh
	db	070h
	db	071h
	db	072h
	db	073h
	db	074h
	db	075h
	db	076h
	db	077h
	db	078h
	db	079h
	db	07ah
	db	05bh
	db	05ch
	db	05dh
	db	05eh
	db	05fh
	db	060h
	db	061h
	db	062h
	db	063h
	db	064h
	db	065h
	db	066h
	db	067h
	db	068h
	db	069h
	db	06ah
	db	06bh
	db	06ch
	db	06dh
	db	06eh
	db	06fh
	db	070h
	db	071h
	db	072h
	db	073h
	db	074h
	db	075h
	db	076h
	db	077h
	db	078h
	db	079h
	db	07ah
	db	07bh
	db	07ch
	db	07dh
	db	07eh
	db	07fh
	db	080h
	db	081h
	db	082h
	db	083h
	db	084h
	db	085h
	db	086h
	db	087h
	db	088h
	db	089h
	db	08ah
	db	08bh
	db	08ch
	db	08dh
	db	08eh
	db	08fh
	db	090h
	db	091h
	db	092h
	db	093h
	db	094h
	db	095h
	db	096h
	db	097h
	db	098h
	db	099h
	db	09ah
	db	09bh
	db	09ch
	db	09dh
	db	09eh
	db	09fh
	db	0a0h
	db	0a1h
	db	0a2h
	db	0a3h
	db	0a4h
	db	0a5h
	db	0a6h
	db	0a7h
	db	0a8h
	db	0a9h
	db	0aah
	db	0abh
	db	0ach
	db	0adh
	db	0aeh
	db	0afh
	db	0b0h
	db	0b1h
	db	0b2h
	db	0b3h
	db	0b4h
	db	0b5h
	db	0b6h
	db	0b7h
	db	0b8h
	db	0b9h
	db	0bah
	db	0bbh
	db	0bch
	db	0bdh
	db	0beh
	db	0bfh
	db	0c0h
	db	0c1h
	db	0c2h
	db	0c3h
	db	0c4h
	db	0c5h
	db	0c6h
	db	0c7h
	db	0c8h
	db	0c9h
	db	0cah
	db	0cbh
	db	0cch
	db	0cdh
	db	0ceh
	db	0cfh
	db	0d0h
	db	0d1h
	db	0d2h
	db	0d3h
	db	0d4h
	db	0d5h
	db	0d6h
	db	0d7h
	db	0d8h
	db	0d9h
	db	0dah
	db	0dbh
	db	0dch
	db	0ddh
	db	0deh
	db	0dfh
	db	0e0h
	db	0e1h
	db	0e2h
	db	0e3h
	db	0e4h
	db	0e5h
	db	0e6h
	db	0e7h
	db	0e8h
	db	0e9h
	db	0eah
	db	0ebh
	db	0ech
	db	0edh
	db	0eeh
	db	0efh
	db	0f0h
	db	0f1h
	db	0f2h
	db	0f3h
	db	0f4h
	db	0f5h
	db	0f6h
	db	0f7h
	db	0f8h
	db	0f9h
	db	0fah
	db	0fbh
	db	0fch
	db	0fdh
	db	0feh
	db	0ffh
const ends

.code
	zend_str_tolower_copy proc
		; rcx char *dest
		; rdx char *src
		; r8  size_t len

		push	r10
		push	r11

		mov	rax,rcx ; save return pointer

		lea	r10,qword ptr [rdx+r8]
		mov	r9,rcx

		cmp	rdx,r10
		jae	done

		lea	r8, offset tolower_map

l1:
		movzx	r11d,byte ptr [rdx]
		
		;cmp	r11b,"a"
		;jae	l0
		;cmp	r11b,"z"
		;jbe	l0


		inc	rdx
		
		
		lea	r9,qword ptr [r9+1]
		movzx	r11d,byte ptr [r11+r8]
		mov	byte ptr [r9-1],r11b
		cmp	rdx,r10
		jb	l1

		mov	byte ptr [r9],0
		pop	r10
		pop	r11
		ret
done:
		mov	byte ptr [rcx],0
		pop	r10
		pop	r11
		ret

	zend_str_tolower_copy endp

end

