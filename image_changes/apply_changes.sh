#!/usr/bin/env bash
set -euo pipefail

BOARD_USER="${BOARD_USER:-root}"
BOARD_IP="${BOARD_IP:-192.168.7.1}"
TARGET="${BOARD_USER}@${BOARD_IP}"

# Works both when called directly and through symlink
SCRIPT_PATH="$(readlink -f "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
TABLE="$SCRIPT_DIR/table.json"

FILTER_NAME="${1:-}"

echo "Target board: $TARGET"
echo "Script path : $SCRIPT_PATH"
echo "Using table : $TABLE"

if [ -n "$FILTER_NAME" ]; then
    echo "Applying only: $FILTER_NAME"
else
    echo "Applying all changes"
fi

echo

if [ ! -f "$TABLE" ]; then
    echo "Error: table.json not found: $TABLE"
    exit 1
fi

if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq is required."
    echo "Install it in WSL with:"
    echo "sudo apt install jq"
    exit 1
fi

echo "Checking SSH connection..."
ssh "$TARGET" "echo 'Connected to devboard.'"
echo

COUNT="$(jq '.files | length' "$TABLE")"
FOUND="false"

for i in $(seq 0 $((COUNT - 1))); do
    NAME="$(jq -r ".files[$i].name" "$TABLE")"

    if [ -n "$FILTER_NAME" ] && [ "$NAME" != "$FILTER_NAME" ]; then
        continue
    fi

    FOUND="true"

    LOCAL_PATH="$(jq -r ".files[$i].local_path" "$TABLE")"
    REMOTE_PATH_RAW="$(jq -r ".files[$i].remote_path" "$TABLE")"
    MODE="$(jq -r ".files[$i].mode" "$TABLE")"
    TYPE="$(jq -r ".files[$i].type" "$TABLE")"

    LOCAL_FILE="$REPO_DIR/$LOCAL_PATH"

    echo "======================================"
    echo "Change: $NAME"
    echo "Type  : $TYPE"
    echo "Local : $LOCAL_FILE"

    if [ ! -f "$LOCAL_FILE" ]; then
        echo "Error: local file not found:"
        echo "$LOCAL_FILE"
        exit 1
    fi

    REMOTE_PATH="$(ssh "$TARGET" "eval echo '$REMOTE_PATH_RAW'")"
    REMOTE_DIR="$(dirname "$REMOTE_PATH")"

    echo "Remote: $REMOTE_PATH"
    echo "Mode  : $MODE"
    echo

    echo "Ensuring remote directory exists:"
    echo "$REMOTE_DIR"
    ssh "$TARGET" "mkdir -p '$REMOTE_DIR'"

    EXISTS="$(ssh "$TARGET" "[ -e '$REMOTE_PATH' ] && echo yes || echo no")"

    if [ "$EXISTS" = "yes" ]; then
        echo
        echo "WARNING: Remote file already exists:"
        echo "$REMOTE_PATH"
        echo
        ssh "$TARGET" "ls -l '$REMOTE_PATH'"
        echo
        read -r -p "Overwrite this file? [y/N]: " ANSWER

        case "$ANSWER" in
            y|Y|yes|YES)
                echo "Overwriting..."
                ;;
            *)
                echo "Skipped: $NAME"
                echo
                continue
                ;;
        esac
    fi

    echo "Copying file..."
    scp "$LOCAL_FILE" "$TARGET:$REMOTE_PATH"

    echo "Setting permissions..."
    ssh "$TARGET" "chmod '$MODE' '$REMOTE_PATH'"

    if [ "$TYPE" = "config" ]; then
        echo "Sourcing config file..."
        ssh "$TARGET" ". '$REMOTE_PATH' && echo 'Config sourced successfully.'"
    fi

    if [ "$TYPE" = "script" ]; then
        echo "Script installed at: $REMOTE_PATH"
    fi

    echo "Done: $NAME"
    echo
done

if [ "$FOUND" = "false" ]; then
    echo "Error: no entry found in table.json with name: $FILTER_NAME"
    exit 1
fi

echo "======================================"
echo "Apply finished."