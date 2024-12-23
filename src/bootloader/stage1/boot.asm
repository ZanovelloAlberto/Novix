org 0x7c00
bits 16

jmp short main
nop

;*********************************************
;    BIOS Parameter Block (BPB) for FAT12
;*********************************************

bpb_oem:					db 'MSWIN4.1'
bpb_bytes_per_sector:		dw 512
bpb_sectors_per_cluster:	db 1
bpb_reserved_sectors:		dw 1
bpb_fat_count:				db 2
bpb_dir_entries_count:		dw 0xe0
bpb_total_sectors:			dw 2880
bpb_media_descriptor_type:	db 0xf0
bpb_sectors_per_fat:		dw 9
bpb_sectors_per_track:		dw 18
bpb_heads:					dw 2
bpb_hidden_sectors:			dd 0
bpb_large_sector_count:		dd 0

ebr_drive_number:			db 0
							db 0
ebr_signature:				db 0x29
ebr_volume_id:				db 0x12, 0x34, 0x56, 0x78
ebr_volume_label:			db "NOVIX      "
ebr_system_id:				db "FAT12   "

main:
	xor ax, ax
	mov ds, ax
	mov es, ax

	mov ss, ax
	mov ax, 0x7c00
	mov sp, ax

	; some BIOSes might start us at 07C0:0000 instead of 0000:7C00, make sure we are in the
    ; expected location
    push es
    push word .codeseg
    retf ; you can also do a far jump

.codeseg:

	; read something from floppy disk
    ; BIOS should set DL to drive number
	mov [ebr_drive_number], dl

	mov si, loading
	call print

	; read drive parameters (sectors per track and head count),
    ; instead of relying on data on formatted disk
    push es
    mov ah, 08h
    int 13h
    jc disk_read.failDiskRead
    pop es

	and cl, 0x3F                        ; remove top 2 bits
    xor ch, ch
    mov [bpb_sectors_per_track], cx     ; sector count

    inc dh
    mov [bpb_heads], dh                 ; head count

	; compute LBA of root directory = reserved + fats * sectors_per_fat
    ; note: this section can be hardcoded
    mov ax, [bpb_sectors_per_fat]
    mov bl, [bpb_fat_count]
    xor bh, bh
    mul bx                              ; ax = (fats * sectors_per_fat)
    add ax, [bpb_reserved_sectors]      ; ax = LBA of root directory
    push ax

    ; compute size of root directory = (32 * number_of_entries) / bytes_per_sector
    mov ax, [bpb_dir_entries_count]
    shl ax, 5                           ; ax *= 32
    xor dx, dx                          ; dx = 0
    div word [bpb_bytes_per_sector]     ; number of sectors we need to read

    test dx, dx                         ; if dx != 0, add 1
    jz .loadFat
    inc ax                              ; division remainder != 0, add 1
                                        ; this means we have a sector only partially filled with entries

;
;loading the root dir table
;
.loadFat:
	
	mov bx, ax
	pop ax
	mov di, ROOTDIR_AND_FAT_OFFSET
	mov dl, [ebr_drive_number]
	call disk_read

;
;searching for the file in the root table
;
	mov si, file_name
	xor bx, bx ; init the counter for .search_file
.search_file:
	push si
	push di
	
	mov cx, 11 ; file name length for fat12 (counter loop for repe)
	repe cmpsb

	pop di
	pop si

	je .file_found

	add di, 32 ; next dir entry
	inc bx
	cmp bx, [bpb_dir_entries_count]
	jl .search_file

	jmp .file_not_found

.file_not_found:
	mov si, msg_file_not_found
	call print
	jmp halt

.file_found:
	mov cx, [di+26] ; first logical cluster filled
	mov [file_cluster], cx

;
;loading the fat table
;
	mov ax, [bpb_reserved_sectors]
	mov bx, [bpb_sectors_per_fat]
	mov di, ROOTDIR_AND_FAT_OFFSET		; we just override the root dir table
	mov dl, [ebr_drive_number]
	call disk_read

	mov ax, BOOT0_SEGMENT
	mov es, ax 		; the file segment here
	mov di, BOOT0_OFFSET 		; the file offset here

.file_cluster_loop:
	mov ax, [file_cluster]
	add ax, 31
	mov bx, [bpb_sectors_per_cluster]
	call disk_read
	add di, [bpb_bytes_per_sector]		;adding the size of the read data to avoid overwriting existing data

	
	mov ax, [file_cluster]
	mov cx, 3
	mul cx
	mov cx, 2
	div cx

	mov si, ROOTDIR_AND_FAT_OFFSET
	add si, ax
	lodsw

	or dx, dx
	jz .even

.odd:
	shr ax, 4
	jmp .nextClusterTest
.even:
	and ax, 0x0fff

.nextClusterTest:
	cmp ax, 0x0ff8
	jae .end_load_file

	mov [file_cluster], ax
	jmp .file_cluster_loop

.end_load_file:
	mov dl, [ebr_drive_number]			; boot device in dl

	mov ax, BOOT0_SEGMENT
	mov ds, ax 							; setting up the data segment of the kernel
	jmp BOOT0_SEGMENT:BOOT0_OFFSET 				;jump to the kernel

halt:
	jmp halt
;=========================================
;read a given number of sector on the disk
;input: 
;	- LBA index in ax
;	- number of sector to read in bx
;	- memory segment in es
;	- memory offset in di
;output: es:di
;=========================================
disk_read:
	push ax
	push bx
	push cx
	push dx
	push si
	push di

	call lba2chs

	mov al, bl
	mov dl, 0x0
	mov bx, di
	mov di, 3 ;counter

.retry_disk:
	stc
	mov ah, 0x2
	int 0x13

	jnc done_read
	call disk_reset

	dec di
	or di, di
	jnz .retry_disk

.failDiskRead:
	mov si, read_failure
	call print
	hlt
	jmp halt

done_read:
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	ret

disk_reset:
	pusha

	mov ah, 0x0
	stc
	int 13h
	jc disk_read.failDiskRead

	popa
	ret

;=========================================
;convert a logical block address to 
;cylinder head sector address
;input: 
;	- LBA index in ax
;output:
;	- cx [bits 0-5]: sector number
;	- cx [bits 6-15]: cylinder
;	- dh: head 
;=========================================
lba2chs:
	push ax

	xor dx, dx
	div word [bpb_sectors_per_track]
	inc dx
	mov cl, dl ;sector

	xor dx, dx
	div word [bpb_heads]
	mov ch, al
	shl ah, 6
	or cl, ah ;cylinder

	shl dx, 8 ;in dh: head

	pop ax
	ret


print:
	push ax
	push bx
	push si
	xor ax, ax
	mov ah, 0xe

print_loop:
	lodsb ;load a single byte from si to al
	or al, al
	jz done_print
	xor bx, bx
	int 0x10
	jmp print_loop

done_print:
	pop si
	pop bx
	pop ax
	ret


loading db "Loading...",13, 10, 0
read_failure DB "failed to read disk !", 13, 10, 0
file_name DB "BOOT0   BIN"
file_cluster DW 0
msg_file_not_found DB "the file boot0.bin doesn't exist !", 13, 10, 0
ROOTDIR_AND_FAT_OFFSET EQU 0xc000
BOOT0_SEGMENT EQU 0x0
BOOT0_OFFSET EQU 0x500
times 510-($-$$) db 0x90
dw 0xaa55