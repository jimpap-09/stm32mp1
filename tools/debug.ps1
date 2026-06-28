<#
.SYNOPSIS
    Αυτόματο helper script για STM32MP1 M4 firmware deployment μέσω SSH/SCP.

.DESCRIPTION
    Το script τρέχει από Windows PowerShell, μέσα από τον φάκελο ενός STM32CubeIDE project.

    Κάνει τα εξής βήματα:
      1. Ψάχνει αναδρομικά μέσα στο project για το πιο πρόσφατο .elf αρχείο που βρίσκεται σε φάκελο Release.
      2. Συνδέεται προσωρινά στο STM32MP1 devboard μέσω SSH.
      3. Δημιουργεί, αν δεν υπάρχει, τον απομακρυσμένο φάκελο /usr/local/m4_examples.
      4. Υπολογίζει SHA256 hash για το τοπικό .elf και για το αντίστοιχο απομακρυσμένο .elf, αν υπάρχει.
      5. Αν τα δύο αρχεία είναι ίδια, δεν κάνει αντιγραφή.
      6. Αν το απομακρυσμένο αρχείο λείπει ή είναι διαφορετικό, το αντιγράφει με SCP και κάνει overwrite.
      7. Ενημερώνει το symlink /lib/firmware/rproc-m4-fw, ώστε να δείχνει στο νέο .elf.
      8. Ανοίγει κανονικό SSH session και σε αφήνει μέσα στο board για να τρέξεις run / stop.

.NOTES
    Αυτό το script ΔΕΝ ξεκινάει από μόνο του τον M4.
    Μόνο ανεβάζει το firmware και ενημερώνει το symlink.

    Το start/stop του M4 γίνεται μετά, μέσα στο devboard, με βοηθητικά commands όπως:
      run
      stop

    Αυτά μπορούν να είναι scripts μέσα στο /usr/local/bin.
#>

param(
    # IP του STM32MP1 devboard.
    # Για USB/Ethernet gadget συνήθως στο δικό μας setup είναι 192.168.7.1.
    [string]$BoardIp = "192.168.7.1",

    # Χρήστης στο Linux του devboard.
    # Στο OpenSTLinux συνήθως δουλεύουμε ως root.
    [string]$User = "root",

    # Φάκελος στο devboard όπου θα αποθηκεύονται τα M4 firmware .elf αρχεία.
    [string]$RemoteDir = "/usr/local/m4_examples",

    # Symlink που διαβάζει το remoteproc firmware loader.
    # Το /lib/firmware/rproc-m4-fw θα δείχνει κάθε φορά στο .elf που μόλις ανεβάσαμε.
    [string]$FirmwareLink = "/lib/firmware/rproc-m4-fw"
)

Write-Host "Searching for Release .elf..."

# Get-ChildItem:
#   Ψάχνει από τον τρέχοντα φάκελο '.' και κάτω, αναδρομικά, όλα τα αρχεία *.elf.
#
# Where-Object:
#   Κρατάει μόνο όσα .elf έχουν στο πλήρες path τους φάκελο \Release\.
#   Το $_ σημαίνει "το τρέχον αντικείμενο" του pipeline.
#   Το $_.FullName είναι το πλήρες path του αρχείου, π.χ. C:\...\Project\Release\Blink.elf.
#
# Sort-Object LastWriteTime -Descending:
#   Ταξινομεί τα .elf από το πιο πρόσφατα τροποποιημένο προς το παλαιότερο.
#
# Select-Object -First 1:
#   Επιλέγει το πρώτο, δηλαδή το πιο πρόσφατο .elf από φάκελο Release.
$elf = Get-ChildItem -Path . -Recurse -Filter *.elf |
    Where-Object { $_.FullName -match "\\Release\\" } |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if (-not $elf) {
    Write-Error "No .elf found inside Release folder. Make sure you have built the STM32CubeIDE project in Release configuration."
    exit 1
}

# Πλήρες τοπικό path του .elf, π.χ. C:\...\Blink\Release\Blink.elf.
$localElf = $elf.FullName

# Μόνο το όνομα του αρχείου, π.χ. Blink.elf.
$elfName = $elf.Name

# Πλήρες path στο devboard όπου θα αποθηκευτεί το firmware.
$remoteElf = "$RemoteDir/$elfName"

# SSH target, π.χ. root@192.168.7.1.
$target = "$User@$BoardIp"

Write-Host "Found ELF: $localElf"
Write-Host "Target: $target"
Write-Host "Remote ELF: $remoteElf"

Write-Host "Creating remote folder..."

# Αυτή η SSH εντολή είναι προσωρινή:
#   1. Ανοίγει SSH σύνδεση.
#   2. Εκτελεί mkdir -p /usr/local/m4_examples.
#   3. Κλείνει SSH και επιστρέφει στο PowerShell.
#
# mkdir -p σημαίνει:
#   - Δημιούργησε τον φάκελο αν δεν υπάρχει.
#   - Αν υπάρχει ήδη, μη βγάλεις error.
ssh $target "mkdir -p '$RemoteDir'"

if ($LASTEXITCODE -ne 0) {
    Write-Error "SSH failed while creating remote folder. Check board IP, Ethernet/USB connection, and SSH availability."
    exit 1
}

Write-Host "Checking hashes..."

# Υπολογίζει SHA256 hash για το τοπικό .elf.
# Το hash είναι σαν "δακτυλικό αποτύπωμα" του αρχείου.
# Αν αλλάξει έστω και λίγο το firmware, π.χ. αλλάξει το HAL_Delay, τότε αλλάζει και το hash.
$localHash = (Get-FileHash -Path $localElf -Algorithm SHA256).Hash.ToLower()

# Υπολογίζει SHA256 hash για το απομακρυσμένο .elf, αν υπάρχει ήδη.
# Αν δεν υπάρχει, η εντολή δεν επιστρέφει hash.
# Το cut κρατάει μόνο το πρώτο πεδίο της sha256sum εξόδου, δηλαδή το ίδιο το hash.
$remoteHash = ssh $target "if [ -f '$remoteElf' ]; then sha256sum '$remoteElf' | cut -d' ' -f1; fi"

# Καθαρίζουμε κενά/newlines ώστε η σύγκριση να είναι αξιόπιστη.
$remoteHash = "$remoteHash".Trim().ToLower()

Write-Host "Local hash : $localHash"
Write-Host "Remote hash: $remoteHash"

# Αν τα hash είναι ίδια, το τοπικό και το απομακρυσμένο .elf είναι ίδια αρχεία.
# Άρα δεν χρειάζεται νέο SCP copy.
#
# Αν άλλαξε το firmware και έγινε νέο build, το localHash θα αλλάξει.
# Τότε το if θα βγει false και θα γίνει overwrite στο devboard με scp.
if ($remoteHash -eq $localHash) {
    Write-Host "Remote ELF is identical. Skipping copy."
} else {
    Write-Host "Copying ELF to devboard..."

    # Το SCP αντιγράφει το τοπικό .elf στο devboard.
    # Τα εισαγωγικά γύρω από $localElf είναι σημαντικά επειδή τα Windows paths μπορεί να έχουν spaces.
    scp "$localElf" "${target}:$remoteElf"

    if ($LASTEXITCODE -ne 0) {
        Write-Error "SCP failed. Check SSH connection and remote directory permissions."
        exit 1
    }
}

Write-Host "Updating firmware symlink..."

# Ενημερώνει το symlink που χρησιμοποιεί το remoteproc.
#
# ln -sfn σημαίνει:
#   -s : φτιάξε symbolic link
#   -f : αν υπάρχει ήδη, αντικατάστησέ το
#   -n : αν ο προορισμός είναι symlink προς directory, χειρίσου το ίδιο το symlink
#
# Μετά το ls -l δείχνει πού δείχνει το symlink.
ssh $target "ln -sfn '$remoteElf' '$FirmwareLink' && ls -l '$FirmwareLink'"

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to update firmware symlink. Check permissions under /lib/firmware."
    exit 1
}

Write-Host ""
Write-Host "Done. Opening SSH session..."
Write-Host "Firmware symlink now points to: $remoteElf"
Write-Host ""

# Εδώ ανοίγει κανονικό SSH session χωρίς να του δώσουμε συγκεκριμένη εντολή.
# Άρα μένουμε μέσα στο Linux του devboard.
# Από εκεί μπορείς να τρέξεις:
#   run
#   stop
ssh $target
