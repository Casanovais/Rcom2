CC = gcc
CFLAGS = -Wall -g # -Wall enables all warnings, -g adds debugging symbols
LDFLAGS =          # Linker flags (none needed for this simple case)

TARGET = download

# List all your .c source files
SRCS = ftp_downloader.c url_parser.c socket_utils.c ftp_utils.c

# Automatically generate .o (object) file names from .c file names
OBJS = $(SRCS:.c=.o)

# The default rule: builds the target executable
all: $(TARGET)

# Rule to link the object files into the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Generic rule to compile .c files into .o files
# $< is the prerequisite (the .c file)
# $@ is the target (the .o file)
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up compiled files
clean:
	rm -f $(OBJS) $(TARGET)

# --- Example Run Targets (Optional, for convenience) ---
# These allow you to type 'make run_netlab_anon' etc.

# Test with NetLab anonymous FTP
run_netlab_anon_pipe: all
	./$(TARGET) ftp://netlab.fe.up.pt/pipe.txt

run_netlab_anon_lslR: all
	./$(TARGET) ftp://netlab.fe.up.pt/debian/ls-lR.gz

# Test with NetLab authenticated FTP
run_netlab_auth_crab: all
	./$(TARGET) ftp://rcom:rcom@netlab.fe.up.pt/files/crab.mp4

# Test with Rebex public FTP
run_rebex: all
	./$(TARGET) ftp://demo:password@test.rebex.net/readme.txt

# Test with ftp.up.pt
run_ftp_up_pt: all
	./$(TARGET) ftp://ftp.up.pt/pub/gnu/emacs/elisp-manual-21-2.8.tar.gz

# Test with speedtest
run_speedtest_100mb: all
	./$(TARGET) ftp://anonymous:anonymous@ftp.bit.nl/speedtest/100mb.bin