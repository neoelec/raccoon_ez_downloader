# Hey Emacs, this is a -*- makefile -*-
# MCU name.
#   mcs51   : the Intel MCS51 family of processors
#   ds390   : the Dallas DS80C390 processor
#   ds400   : the Dallas DS80C400 processor
#   hc08    : the Freescale/Motorola HC08 family of processors
#   s08     : the Freescale/Motorola S08 (aka 68HCS08, HCS08, CS08) family of processors
#   z80     : the Zilog Z80 family of processors
#   z180    : the Zilog Z180 family of processors
#   r2k     : the Rabbit 2000 / Rabbit 3000 family of processors
#   r3ka    : the Rabbit 3000A family of processors
#   gbz80   : the LR35902 GameBoy Z80 processor
#   tlcs90  : the Toshiba TLCS-89 processor
#   ez80_z80: the Zilog eZ80 processor in Z80 mode
#   stm8    : the STMicroelectronics STM8 family of processors
#   pdk14   : the Padauk processor with 14 bit wide program memory
#   pdk15   : the Padauk processor with 15 bit wide program memory
#   pic14   : the Microchip PIC 14-bit processors (in development, not complete)
#   pic16   : the Microchip PIC 16-bit processors (in development, not complete)
MCU		:= mcs51

# Assembler MCU name.
#   390     :
#   6808    :
#   8051    :
#   gb      :
#   z80     :
#   rab     :
#   stm8    :
#   tlcs90  ;
ASMCU		:= 8051

# Processor frequency.
F_CPU		:= 11059200


# Output FORMAT.
#   ihx : Intel Hex FORMAT
#   s19 : Motorola S19 FORMAT
#   elf : ELF FORMAT
FORMAT		:= ihx


# Target file name
TARGET		?= ez20


# VPATH variable
VPATH		:=


# Define all C sorce files.
CSRCS		:= $(TARGET).c ez_common.c


# Define all Assembler source files.
ASRCS		:= $(EXTRA_ASRCS)


# Define TTY Device.
TTY_DEV		:= /dev/ttyUSB0


# Output directories
BINDIR		:= bin
OBJDIR		:= obj


# Optimization options here.
OPT		:= --opt-code-size


# List any extra directories to look for include files here.
EXTRAINCDIRS	:=


# Compiler flag to set the C Standard level.
#   sdcc89  : ISO C90 and ANSI C89 + SDCC behaviour
#   c89     : ISO C90 and ANSI C89
#   sdcc99  : ISO C99 + SDCC behaviour
#   c99     : ISO C99
#   sdcc11  : ISO C11 + SDCC behaviour
#   c11     : ISO C11
CSTANDARD	:= sdcc11


# Place -I options here
CINCS		:=


# MCU specific options here.
MCU_CFLAGS	:=
MCU_LDFLAGS	:=

# Place -D or -U options here
CDEFS		:= -DF_CPU=$(F_CPU)UL


# Compiler Options
CFLAGS		:= $(CDEFS) -m$(MCU) $(OPT) --std-$(CSTANDARD)
#CFLAGS		+= --debug
CFLAGS		+= $(CINCS)
CFLAGS		+= $(patsubst %,-I%,$(EXTRAINCDIRS))
CFLAGS		+= $(MCU_CFLAGS)
CFLAGS		+= --verbose


# Assembler Options
ASFLAGS		:= -x -l -s
ASFLAGS		+= -j -y
ASFLAGS		+= $(patsubst %,-I%,$(EXTRAINCDIRS))


# The start location of the external ram, default value is 0.
XRAM_LOC	:= --xram-loc 0x0000


# The start location of the code segment, default value is 0.
CODE_LOC	:= --code-loc 0x0000


LDFLAGS		:= --out-fmt-$(FORMAT) $(OPT)
LDFLAGS		+= $(XRAM_LOC)
LDFLAGS		+= $(CODE_LOC)
LDFLAGS		+= $(MCU_LDFLAGS)
LDFLAGS		+= --verbose


# Define programs and commands.
CC		:= sdcc
AS		:= sdas$(ASMCU)
REMOVE		:= rm -rf
PACKIHX		:= packihx

define run_packihx
	cat $(1) | sed -e '/^:00000001FF/d' | perl -p -e 's/^(:.{2})/$$1 /' |\
		sort -k2 | sed -e 's/ //' | $(PACKIHX) > $(2);		\
	echo ':00000001FF' >> $(2)
endef

define upload_packihx
	for char in `cat $(1) | perl -p -e 's/(\S{1})(\S{1})/$$1 $$2 /g'`; do \
		echo -n $$char > $(2); \
		sleep 0.005; \
	done
endef

# Define all object files.
COBJS		:= $(addprefix $(OBJDIR)/, $(CSRCS:.c=.rel))
AOBJS		:= $(addprefix $(OBJDIR)/, $(ASRCS:.S=.rel))


# Define output file
OUTPUT		:= $(addprefix $(BINDIR)/, $(TARGET).$(FORMAT))


# Define Messages
MSG_BEGIN	:= -------- begin --------
MSG_END		:= --------  end  --------
MSG_LINKING	:= Linking:
MSG_COMPILING	:= Compiling:
MSG_ASSEMBLING	:= Assembling:
MSG_CLEANING	:= Cleaning project:


# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS	:= -I. $(CFLAGS)
ALL_ASFLAGS	:= -I. $(ASFLAGS)


all: build


build: sdcc-version begin $(BINDIR) $(OBJDIR) $(OUTPUT) end


sdcc-version:
	$(CC) --version


begin:
	@echo
	@echo $(MSG_BEGIN)


end:
	@echo
	@echo $(MSG_END)
	@echo


$(BINDIR) $(OBJDIR):
	mkdir $@


$(OUTPUT): $(COBJS) $(AOBJS)
	@echo
	@echo $(MSG_LINKING)
	$(CC) $(LDFLAGS) -o $@ $^
	@if [ ihx = $(FORMAT) ]; then $(call run_packihx,$@,$(@:.ihx=.hex)); fi


# Compile: create object files from C source files.
$(COBJS): $(OBJDIR)/%.rel: %.c
	@echo
	@echo $(MSG_COMPILING)
	$(CC) -c $(ALL_CFLAGS) -o $@ $<


# Assemble: create object files from assembler source files
$(AOBJS): $(OBJDIR)/%.rel: %.S
	@echo
	@echo $(MSG_ASSEMBLING) $<
	$(AS) $(ALL_ASFLAGS) -o $@ $<


upload: $(OUTPUT)
	@if [ ihx = $(FORMAT) ]; then $(call upload_packihx,$(<:.ihx=.hex),$(TTY_DEV)); fi


# Target: clean project.
clean: begin clean_list end


clean_list:
	@echo
	@echo $(MSG_CLEANING)
	$(REMOVE) $(BINDIR)
	$(REMOVE) $(OBJDIR)


# Listing of phony targets.
.PHONY : clean begin all end sdcc-version upload
