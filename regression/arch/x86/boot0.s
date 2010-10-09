# [oformat elf32]
# 1 "boot0.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "boot0.S"
# 132 "boot0.S"
  .set NHRDRV,0x475 # Number of hard drives
  .set ORIGIN,0x600 # Execution address
  .set LOAD,0x7c00 # Load address

  .set PRT_OFF,0x1be # Partition table
  .set B0_OFF,(0x1ae -0x200) # Offset of boot0 data

  .set MAGIC,0xaa55 # Magic: bootable

  .set KEY_ENTER,0x1c # Enter key scan code
  .set KEY_F1,0x3b # F1 key scan code
  .set KEY_1,0x02 # #1 key scan code

  .set ASCII_BEL,'#' # ASCII code for <BEL>
  .set ASCII_CR,0x0D # ASCII code for <CR>







  .set _NXTDRV, B0_OFF+6 # Next drive
  .set _OPT, B0_OFF+7 # Default option
  .set _SETDRV, B0_OFF+8 # Drive to force
  .set _FLAGS, B0_OFF+9 # Flags
  .set SETDRV, 0x20 # the 'setdrv' flag
  .set NOUPDATE, 0x40 # the 'noupdate' flag
  .set USEPACKET, 0x80 # the 'packet' flag


  .set _TICKS, (PRT_OFF - 0x200 - 2) # Timeout ticks
  .set _MNUOPT, 0x10 # Saved menu entries

  .set TLEN, (desc_ofs - bootable_ids) # size of bootable ids
  .globl start # Entry point
  .code16 # This runs in real mode
# 181 "boot0.S"
start: cld # String ops inc
  xorw %ax,%ax # Zero
  movw %ax,%es # Address
  movw %ax,%ds # data
  movw %ax,%ss # Set up
  movw $LOAD,%sp # stack




  movw %sp,%si # Source
  movw $start,%di # Destination
  movw $0x100,%cx # Word count
  rep # Relocate
  movsw # code






  movw %di,%bp # Address variables
  movb $0x8,%cl # Words to clear
  rep # Zero
  stosw # them
  incb -0xe(%di) # Set the S field to 1

  jmp main-LOAD+ORIGIN # Jump to relocated code

main:
# 227 "boot0.S"
  testb $SETDRV,_FLAGS(%bp) # Set drive number?

  jz save_curdrive # no, use the default
# 238 "boot0.S"
disable_update: orb $NOUPDATE,_FLAGS(%bp) # Disable updates
  movb _SETDRV(%bp),%dl # Use stored drive number







save_curdrive: movb %dl, (%bp) # Save drive number
  pushw %dx # Also in the stack





  callw putn # Print a newline






  movw $(partbl+0x4),%bx # Partition table (+4)
  xorw %dx,%dx # Item number





read_entry: movb %ch,-0x4(%bx) # Zero active flag (ch == 0)
  btw %dx,_FLAGS(%bp) # Entry enabled?
  jnc next_entry # No
  movb (%bx),%al # Load type
  test %al, %al # skip empty partition
  jz next_entry







  movw $bootable_ids,%di # Lookup tables
  movb $(TLEN+1),%cl # Number of entries
  repne # Locate
  scasb # type





  addw $(TLEN-1), %di # Adjust
  movb (%di),%cl # Partition
  addw %cx,%di # description
  callw putx # Display it

next_entry: incw %dx # Next item
  addb $0x10,%bl # Next entry
  jnc read_entry # Till done






  popw %ax # Drive number
  subb $0x80-0x1,%al # Does next
  cmpb NHRDRV,%al # drive exist? (from BIOS?)
  jb print_drive # Yes



  decw %ax # Already drive 0?
  jz print_prompt # Yes



  xorb %al,%al # Drive 0






print_drive: addb $'0'|0x80,%al # Save next
  movb %al,_NXTDRV(%bp) # drive number
  movw $drive,%di # Display
  callw putx # item





print_prompt: movw $prompt,%si # Display
  callw putstr # prompt
  movb _OPT(%bp),%dl # Display
  decw %si # default
  callw putkey # key
  jmp start_input # Skip beep




beep: movb $ASCII_BEL,%al # Input error, print or beep
  callw putchr

start_input:



  xorb %ah,%ah # BIOS: Get
  int $0x1a # system time
  movw %dx,%di # Ticks when
  addw _TICKS(%bp),%di # timeout
read_key:




  movb $0x1,%ah # BIOS: Check
  int $0x16 # for keypress






  jnz got_key # Have input
  xorb %ah,%ah # BIOS: int 0x1a, 00
  int $0x1a # get system time
  cmpw %di,%dx # Timeout?
  jb read_key # No




use_default: movb _OPT(%bp),%al # Load default
  orb $NOUPDATE,_FLAGS(%bp) # Disable updates
  jmp check_selection # Join common code
# 386 "boot0.S"
got_key:

  xorb %ah,%ah # BIOS: int 0x16, 00
  int $0x16 # get keypress
  movb %ah,%al # move scan code to %al
  cmpb $KEY_ENTER,%al





  je use_default # enter -> default
# 408 "boot0.S"
  subb $KEY_F1,%al

  cmpb $0x5,%al # F1..F6
  jna 3f # Yes
  subb $(KEY_1 - KEY_F1),%al # Less #1 scan code
 3:


  cmpb $0x5,%al # F1..F6 or 1..6 ?

  jne 1f;
  int $0x18 # found F6, try INT18
 1:

  jae beep # Not in F1..F5, beep

check_selection:





  cbtw # Extend (%ah=0 used later)
  btw %ax,_MNUOPT(%bp) # Option enabled?
  jnc beep # No




  movb %al,_OPT(%bp) # Save option
# 446 "boot0.S"
  movw %bp,%si # Partition for write
  movb (%si),%dl # Drive number, saved above
  movw %si,%bx # Partition for read
  cmpb $0x4,%al # F5/#5 pressed?
  pushf # Save results for later
  je 1f # Yes, F5





  shlb $0x4,%al # Point to
  addw $partbl,%ax # selected
  xchgw %bx,%ax # partition
  movb $0x80,(%bx) # Flag active





 1: pushw %bx # Save
  testb $NOUPDATE,_FLAGS(%bp) # No updates?
  jnz 2f # skip update
  movw $start,%bx # Data to write
  movb $0x3,%ah # Write sector
  callw intx13 # to disk
 2: popw %si # Restore





  popf # Restore %al test results
  jne 3f # If not F5/#5
  movb _NXTDRV(%bp),%dl # Next drive
  subb $'0',%dl # number




 3: movw $LOAD,%bx # Address for read
  movb $0x2,%ah # Read sector
  callw intx13 # from disk
  jc beep # If error
  cmpw $MAGIC,0x1fe(%bx) # Bootable?
  jne beep # No
  pushw %si # Save ptr to selected part.
  callw putn # Leave some space
  popw %si # Restore, next stage uses it
  jmp *%bx # Invoke bootstrap
# 513 "boot0.S"
putx: btsw %dx,_MNUOPT(%bp) # Enable menu option
  movw $item,%si # Display
  callw putkey # key
  movw %di,%si # Display the rest
  callw putstr # Display string

putn: movw $crlf,%si # To next line
  jmp putstr

putkey:

  movb $'F',%al # Display
  callw putchr # 'F'

  movb $'1',%al # Prepare
  addb %dl,%al # digit

putstr.1: callw putchr # Display char
putstr: lodsb # Get byte
  testb $0x80,%al # End of string?
  jz putstr.1 # No
  andb $~0x80,%al # Clear MSB then print last

putchr:

  pushw %bx # Save
  movw $0x7,%bx # Page:attribute
  movb $0xe,%ah # BIOS: Display
  int $0x10 # character
  popw %bx # Restore
# 551 "boot0.S"
  retw # To caller
# 561 "boot0.S"
intx13: # Prepare CHS parameters
  movb 0x1(%si),%dh # Load head
  movw 0x2(%si),%cx # Load cylinder:sector
  movb $0x1,%al # Sector count
  pushw %si # Save
  movw %sp,%di # Save

  testb %dl, %dl # is this a floppy ?
  jz 1f # Yes, use CHS mode

  testb $USEPACKET,_FLAGS(%bp) # Use packet interface?
  jz 1f # No
  pushl $0x0 # Set the
  pushl 0x8(%si) # LBA address
  pushw %es # Set the transfer
  pushw %bx # buffer address
  push $0x1 # Block count
  push $0x10 # Packet size
  movw %sp,%si # Packet pointer
  decw %ax # Verify off
  orb $0x40,%ah # Use disk packet
 1: int $0x13 # BIOS: Disk I/O
  movw %di,%sp # Restore
  popw %si # Restore
  retw # To caller





prompt:

  .ascii "\nF6 PXE\r"

  .ascii "\nBoot:"
item: .ascii " "; .byte ' '|0x80
crlf: .ascii "\r"; .byte '\n'|0x80



bootable_ids:





  .byte 0x83, 0xa5, 0xa6, 0xa9, 0x06, 0x07, 0x0b
# 616 "boot0.S"
desc_ofs:





  .byte os_linux-. # 131, Linux
  .byte os_freebsd-. # 165, FreeBSD
  .byte os_bsd-. # 166, OpenBSD
  .byte os_bsd-. # 169, NetBSD
  .byte os_dos-. # 6, FAT16 >= 32M
  .byte os_win-. # 7, NTFS
  .byte os_win-. # 11, FAT32
# 637 "boot0.S"
  .byte os_misc-. # Unknown





os_misc: .byte '?'|0x80
os_dos:



os_win: .ascii "Wi"; .byte 'n'|0x80
os_linux: .ascii "Linu"; .byte 'x'|0x80
os_freebsd: .ascii "Free"
os_bsd: .ascii "BS"; .byte 'D'|0x80




  .org (0x200 + B0_OFF),0x90
# 665 "boot0.S"
drive: .ascii "Drive "
nxtdrv: .byte 0x0 # Next drive number
opt: .byte 0x0 # Option
setdrv_num: .byte 0x80 # Drive to force
flags: .byte 0x8f # Flags

  .byte 0xa8,0xa8,0xa8,0xa8 # Volume Serial Number

ticks: .word 0xb6 # Delay

  .org PRT_OFF



partbl: .fill 0x40,0x1,0x0 # Partition table
  .word MAGIC # Magic number
  .org 0x200 # again, safety check
endblock:
