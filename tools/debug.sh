#!/bin/bash

# ============================================================
# STM32MP1 M4 remoteproc debug helper - Linux / WSL version
# ============================================================
#
# Αυτό το script κάνει την ίδια δουλειά με το debug.ps1,
# αλλά τρέχει από WSL/Linux.
#
# Workflow:
# 1. Ψάχνει μέσα στο τρέχον STM32CubeIDE project για .elf αρχείο
#    που βρίσκεται μέσα σε φάκελο Release.
# 2. Παίρνει το πιο πρόσφατο Release .elf.
# 3. Συνδέεται μέσω SSH στο STM32MP1 devboard.
# 4. Δημιουργεί τον remote φάκελο /usr/local/m4_examples αν δεν υπάρχει.
# 5. Συγκρίνει το local .elf με το remote .elf μέσω SHA256 hash.
# 6. Αν είναι ίδια, δεν ξανακάνει copy.
# 7. Αν είναι διαφορετικά ή δεν υπάρχει remote αρχείο, κάνει scp overwrite.
# 8. Αλλάζει το symlink /lib/firmware/rproc-m4-fw ώστε να δείχνει στο νέο .elf.
# 9. Ανοίγει interactive SSH session και σε αφήνει μέσα στο board.
#
# Χρήση:
#   cd /mnt/c/path/to/STM32CubeIDE/project
#   ~/github/stm32mp1/tools/debug.sh
#
# Με custom IP:
#   ~/github/stm32mp1/tools/debug.sh -i 192.168.7.1
#
# ============================================================


# -----------------------------
# Default parameters
# -----------------------------
# Αυτές είναι οι default τιμές. Μπορούν να αλλάξουν με options.
BOARD_IP="192.168.7.1"
USER="root"
REMOTE_DIR="/usr/local/m4_examples"
FIRMWARE_LINK="/lib/firmware/rproc-m4-fw"


# -----------------------------
# Parse command-line arguments
# -----------------------------
# Επιτρέπουμε να αλλάζεις IP, user, remote dir ή firmware link από την εντολή.
#
# Παράδειγμα:
#   ./debug.sh -i 192.168.7.1 -u root
#
while getopts "i:u:d:l:h" opt; do
    case "$opt" in
        i)
            BOARD_IP="$OPTARG"
            ;;
        u)
            USER="$OPTARG"
            ;;
        d)
            REMOTE_DIR="$OPTARG"
            ;;
        l)
            FIRMWARE_LINK="$OPTARG"
            ;;
        h)
            echo "Usage: $0 [-i board_ip] [-u user] [-d remote_dir] [-l firmware_link]"
            echo ""
            echo "Defaults:"
            echo "  board_ip      = $BOARD_IP"
            echo "  user          = $USER"
            echo "  remote_dir    = $REMOTE_DIR"
            echo "  firmware_link = $FIRMWARE_LINK"
            exit 0
            ;;
        *)
            echo "Invalid option."
            exit 1
            ;;
    esac
done


# -----------------------------
# Helper function for errors
# -----------------------------
# Αν κάτι πάει λάθος, τυπώνουμε μήνυμα και σταματάμε το script.
fail() {
    echo "ERROR: $1"
    exit 1
}


echo "Searching for Release .elf..."


# -----------------------------
# Find latest Release .elf
# -----------------------------
# Το find ψάχνει από το current directory "." και κάτω.
#
# Θέλουμε μόνο .elf αρχεία που βρίσκονται σε path που περιέχει /Release/.
#
# Το printf '%T@ %p\n' τυπώνει:
#   timestamp path
#
# Μετά:
#   sort -nr      -> βάζει πρώτο το πιο πρόσφατο αρχείο
#   head -n 1     -> παίρνει το πρώτο
#   cut -d' ' -f2- -> αφαιρεί το timestamp και κρατάει μόνο το path
#
# Άρα αν υπάρχουν πολλά .elf, παίρνουμε το πιο πρόσφατο Release .elf.
LOCAL_ELF="$(
    find . -type f -name "*.elf" -path "*/Release/*" -printf '%T@ %p\n' 2>/dev/null |
    sort -nr |
    head -n 1 |
    cut -d' ' -f2-
)"


# Αν δεν βρεθεί .elf μέσα σε Release φάκελο, σταματάμε.
if [ -z "$LOCAL_ELF" ]; then
    echo ""
    echo "WARNING: No .elf found inside Release folder."
    echo "Searching for Debug .elf instead..."
    echo ""

    LOCAL_ELF="$(
        find . -type f -name "*.elf" -path "*/Debug/*" -printf '%T@ %p\n' 2>/dev/null |
        sort -nr |
        head -n 1 |
        cut -d' ' -f2-
    )"
fi

if [ -z "$LOCAL_ELF" ]; then
    fail "No .elf found inside Release or Debug folder. Build the project first."
fi

# -----------------------------
# Build useful variables
# -----------------------------
# LOCAL_ELF:
#   Το path του .elf στο PC/WSL.
#
# ELF_NAME:
#   Μόνο το όνομα του αρχείου, χωρίς path.
#
# REMOTE_ELF:
#   Το path στο devboard όπου θα αντιγραφεί το .elf.
#
# TARGET:
#   Ο SSH στόχος, π.χ. root@192.168.7.1.
#
ELF_NAME="$(basename "$LOCAL_ELF")"
REMOTE_ELF="$REMOTE_DIR/$ELF_NAME"
TARGET="$USER@$BOARD_IP"


echo "Found ELF: $LOCAL_ELF"
echo "Target: $TARGET"
echo "Remote ELF: $REMOTE_ELF"


# -----------------------------
# Create remote directory
# -----------------------------
# Αυτό ανοίγει προσωρινό SSH connection, τρέχει mkdir -p και κλείνει.
#
# mkdir -p σημαίνει:
#   - φτιάξε τον φάκελο αν δεν υπάρχει
#   - αν υπάρχει ήδη, μη βγάλεις error
#
echo "Checking remote folder..."

REMOTE_DIR_STATUS="$(
    ssh "$TARGET" "
        if [ -d '$REMOTE_DIR' ]; then
            echo exists
        else
            mkdir -p '$REMOTE_DIR' && echo created
        fi
    "
)"

REMOTE_DIR_STATUS="$(echo "$REMOTE_DIR_STATUS" | tr -d '\r' | tail -n 1)"

if [ "$REMOTE_DIR_STATUS" = "exists" ]; then
    echo "Remote folder already exists: $REMOTE_DIR"
elif [ "$REMOTE_DIR_STATUS" = "created" ]; then
    echo "Created remote folder: $REMOTE_DIR"
else
    fail "Failed to check or create remote folder: $REMOTE_DIR"
fi


# -----------------------------
# Calculate local hash
# -----------------------------
# Το SHA256 hash είναι σαν δακτυλικό αποτύπωμα του αρχείου.
#
# Αν αλλάξει έστω και λίγο το firmware, π.χ. αλλάξεις HAL_Delay,
# τότε μετά από rebuild θα αλλάξει το .elf και άρα θα αλλάξει και το hash.
#
echo "Checking hashes..."

LOCAL_HASH="$(sha256sum "$LOCAL_ELF" | cut -d' ' -f1 | tr '[:upper:]' '[:lower:]')"


# -----------------------------
# Calculate remote hash
# -----------------------------
# Εδώ συνδεόμαστε προσωρινά στο devboard και ελέγχουμε:
#
# Αν υπάρχει ήδη το remote .elf:
#   υπολόγισε το sha256sum του
#
# Αν δεν υπάρχει:
#   άφησε το REMOTE_HASH κενό
#
REMOTE_HASH="$(
    ssh "$TARGET" "if [ -f '$REMOTE_ELF' ]; then sha256sum '$REMOTE_ELF' | cut -d' ' -f1; fi" |
    tr -d '\r' |
    tr '[:upper:]' '[:lower:]'
)"


echo "Local hash : $LOCAL_HASH"
echo "Remote hash: $REMOTE_HASH"


# -----------------------------
# Copy only if needed
# -----------------------------
# Αν το local hash είναι ίδιο με το remote hash,
# τότε το ίδιο ακριβώς .elf υπάρχει ήδη στο devboard.
#
# Άρα δεν χρειάζεται να κάνουμε ξανά scp.
#
# Αν όμως:
#   - δεν υπάρχει remote αρχείο
#   - ή υπάρχει αλλά είναι διαφορετικό
#
# τότε κάνουμε scp και το αντικαθιστούμε.
#
if [ "$REMOTE_HASH" = "$LOCAL_HASH" ]; then
echo "Remote ELF is identical. Skipping copy."
echo "Firmware symlink already points to the same ELF. Skipping update."
else
echo "Copying ELF to devboard..."
scp "$LOCAL_ELF" "$TARGET:$REMOTE_ELF" || fail "SCP failed."

```
echo "Updating firmware symlink..."
ssh "$TARGET" "ln -sfn '$REMOTE_ELF' '$FIRMWARE_LINK' && ls -l '$FIRMWARE_LINK'" \
    || fail "Failed to update firmware symlink."
```

fi



#
# Εμείς κρατάμε τα πραγματικά .elf στο:
#   /usr/local/m4_examples
#
# και φτιάχνουμε symlink:
#   /lib/firmware/rproc-m4-fw -> /usr/local/m4_examples/<file>.elf
#
# Το ln -sfn σημαίνει:
#   -s : symbolic link
#   -f : force overwrite existing link/file
#   -n : αν ο προορισμός είναι symlink, μην ακολουθήσεις το symlink
#
echo "Updating firmware symlink..."
ssh "$TARGET" "ln -sfn '$REMOTE_ELF' '$FIRMWARE_LINK' && ls -l '$FIRMWARE_LINK'" \


# -----------------------------
# Done - open SSH session
# -----------------------------
# Μέχρι εδώ το script:
#   - βρήκε το Release .elf
#   - το σύγκρινε με το remote
#   - το αντέγραψε μόνο αν χρειαζόταν
#   - ενημέρωσε το firmware symlink
#
echo ""
echo "Done."
echo "Firmware symlink now points to: $REMOTE_ELF"
echo ""
