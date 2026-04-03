#!/usr/bin/env bash
set -euo pipefail

SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SV_DIALECT_SOURCE="${SCRIPT_DIR}/sv_mavlink_dialect.xml"
MAVLINK_REPO_URL="https://github.com/mavlink/mavlink.git"

TARGET_DIR=""
OUTPUT_PATH=""
LANGUAGE="C"
MAVGEN_LANGUAGE="C"
MAVGEN_PYTHON=""

usage() {
    cat <<EOF
Usage:
  ${SCRIPT_NAME} --target <mavlink-dir> --output <path> [--lang <C|Python>]

Description:
  Clone (or reuse) the official MAVLink repository, copy this repo's
  sv_mavlink_dialect.xml into MAVLink message_definitions/v1.0/, ensure
  all.xml includes sv_mavlink_dialect.xml, and run mavgen using:
    message_definitions/v1.0/all.xml

Arguments:
  -t, --target <mavlink-dir>
      Path where the MAVLink repo will be cloned, or an existing MAVLink
      clone to reuse.

  -o, --output <path>
      Output path for generated bindings.
      For --lang C, this must be a directory path.
      For --lang Python, this must be a file path (example: ./sv_mavlink.py).

  -l, --lang <C|Python>
      Generation language (default: C).

  -h, --help
      Show this help text.
EOF
}

die() {
    printf 'Error: %s\n' "$1" >&2
    exit 1
}

require_command() {
    local command_name="$1"
    if ! command -v "$command_name" >/dev/null 2>&1; then
        die "Required command not found: ${command_name}"
    fi
}

to_absolute_path() {
    local input_path="$1"
    python3 - "$input_path" <<'PY'
import os
import sys

print(os.path.abspath(sys.argv[1]))
PY
}

is_mavlink_clone() {
    local candidate_dir="$1"
    [[ -d "$candidate_dir" ]] \
        && [[ -e "$candidate_dir/.git" ]] \
        && [[ -f "$candidate_dir/message_definitions/v1.0/all.xml" ]]
}

ensure_mavlink_clone() {
    local target_dir="$1"

    if is_mavlink_clone "$target_dir"; then
        printf 'Reusing existing MAVLink clone: %s\n' "$target_dir"
    else
        if [[ -e "$target_dir" ]]; then
            [[ -d "$target_dir" ]] || die "Target path exists but is not a directory: ${target_dir}"

            if [[ -n "$(ls -A "$target_dir")" ]]; then
                die "Target directory exists but is not a MAVLink clone: ${target_dir}"
            fi
        fi

        mkdir -p "$(dirname "$target_dir")"
        printf 'Cloning official MAVLink repository into: %s\n' "$target_dir"
        git clone --recursive "$MAVLINK_REPO_URL" "$target_dir"

        if ! is_mavlink_clone "$target_dir"; then
            die "Clone completed but target does not look like a MAVLink clone: ${target_dir}"
        fi
    fi

    if [[ ! -f "$target_dir/pymavlink/requirements.txt" ]]; then
        printf 'Initializing MAVLink submodules...\n'
        git -C "$target_dir" submodule update --init --recursive
    fi

    [[ -f "$target_dir/pymavlink/requirements.txt" ]] \
        || die "Missing pymavlink requirements at ${target_dir}/pymavlink/requirements.txt"
}

copy_sv_dialect() {
    local mavlink_dir="$1"
    local destination_file="${mavlink_dir}/message_definitions/v1.0/sv_mavlink_dialect.xml"

    [[ -f "$SV_DIALECT_SOURCE" ]] || die "Dialect file not found: ${SV_DIALECT_SOURCE}"

    if [[ -f "$destination_file" ]] && cmp -s "$SV_DIALECT_SOURCE" "$destination_file"; then
        printf 'Dialect already up to date: %s\n' "$destination_file"
        return
    fi

    cp "$SV_DIALECT_SOURCE" "$destination_file"
    printf 'Copied dialect XML to: %s\n' "$destination_file"
}

ensure_all_xml_include() {
    local all_xml_file="$1"
    local include_pattern='<include>[[:space:]]*sv_mavlink_dialect\.xml[[:space:]]*</include>'
    local include_line='  <include>sv_mavlink_dialect.xml</include>'

    if grep -Eq "$include_pattern" "$all_xml_file"; then
        printf 'Include already present in all.xml\n'
        return
    fi

    local temp_file
    temp_file="$(mktemp)"

    if ! awk -v include_line="$include_line" '
        /<\/mavlink>/ && !inserted {
            print include_line
            inserted = 1
        }
        { print }
        END {
            if (!inserted) {
                exit 1
            }
        }
    ' "$all_xml_file" >"$temp_file"; then
        rm -f "$temp_file"
        die "Unable to insert include into ${all_xml_file}"
    fi

    mv "$temp_file" "$all_xml_file"
    printf 'Added include to all.xml: <include>sv_mavlink_dialect.xml</include>\n'
}

venv_has_working_mavgen() {
    local mavlink_dir="$1"
    local python_bin="$2"

    [[ -x "$python_bin" ]] || return 1

    (
        cd "$mavlink_dir"
        "$python_bin" -m pymavlink.tools.mavgen --help >/dev/null 2>&1
    )
}

ensure_mavgen_environment() {
    local mavlink_dir="$1"
    local venv_dir="${mavlink_dir}/.venv"
    local venv_python="${venv_dir}/bin/python3"
    local requirements_file="${mavlink_dir}/pymavlink/requirements.txt"

    if venv_has_working_mavgen "$mavlink_dir" "$venv_python"; then
        printf 'Using existing MAVLink virtual environment: %s\n' "$venv_dir"
        MAVGEN_PYTHON="$venv_python"
        return
    fi

    if [[ ! -x "$venv_python" ]]; then
        printf 'Creating MAVLink virtual environment: %s\n' "$venv_dir"
        python3 -m venv "$venv_dir"
    else
        printf 'Existing MAVLink virtual environment is not usable; refreshing dependencies.\n'
    fi

    printf 'Installing pymavlink requirements...\n'
    "$venv_python" -m pip install -r "$requirements_file"

    if ! venv_has_working_mavgen "$mavlink_dir" "$venv_python"; then
        die "mavgen is still not runnable after preparing ${venv_dir}"
    fi

    printf 'MAVLink virtual environment ready: %s\n' "$venv_dir"
    MAVGEN_PYTHON="$venv_python"
}

validate_output_path() {
    local output_path="$1"
    local language="$2"

    if [[ "$language" == "C" ]]; then
        if [[ -e "$output_path" && ! -d "$output_path" ]]; then
            die "For --lang C, --output must be a directory path: ${output_path}"
        fi
        mkdir -p "$output_path"
        return
    fi

    if [[ -e "$output_path" && -d "$output_path" ]]; then
        die "For --lang Python, --output must be a file path, not a directory: ${output_path}"
    fi
    mkdir -p "$(dirname "$output_path")"
}

run_mavgen() {
    local mavlink_dir="$1"
    local output_path="$2"
    local language="$3"

    printf 'Running mavgen (language=%s)...\n' "$language"
    (
        cd "$mavlink_dir"
        "$MAVGEN_PYTHON" -m pymavlink.tools.mavgen \
            --lang="$language" \
            --wire-protocol=2.0 \
            --output="$output_path" \
            message_definitions/v1.0/all.xml
    )
    printf 'Generation finished. Output: %s\n' "$output_path"
}

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -t|--target)
                [[ $# -ge 2 ]] || die "Missing value for ${1}"
                TARGET_DIR="$2"
                shift 2
                ;;
            --target=*)
                TARGET_DIR="${1#*=}"
                shift
                ;;
            -o|--output)
                [[ $# -ge 2 ]] || die "Missing value for ${1}"
                OUTPUT_PATH="$2"
                shift 2
                ;;
            --output=*)
                OUTPUT_PATH="${1#*=}"
                shift
                ;;
            -l|--lang)
                [[ $# -ge 2 ]] || die "Missing value for ${1}"
                LANGUAGE="$2"
                shift 2
                ;;
            --lang=*)
                LANGUAGE="${1#*=}"
                shift
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                die "Unknown argument: ${1}. Use --help for usage."
                ;;
        esac
    done

    [[ -n "$TARGET_DIR" ]] || die "Missing required argument: --target"
    [[ -n "$OUTPUT_PATH" ]] || die "Missing required argument: --output"

    case "${LANGUAGE,,}" in
        c)
            MAVGEN_LANGUAGE="C"
            ;;
        python|py)
            MAVGEN_LANGUAGE="Python"
            ;;
        *)
            die "Unsupported --lang value: ${LANGUAGE}. Use C or Python."
            ;;
    esac
}

main() {
    parse_arguments "$@"

    require_command git
    require_command python3

    TARGET_DIR="$(to_absolute_path "$TARGET_DIR")"
    OUTPUT_PATH="$(to_absolute_path "$OUTPUT_PATH")"

    ensure_mavlink_clone "$TARGET_DIR"
    copy_sv_dialect "$TARGET_DIR"
    ensure_all_xml_include "$TARGET_DIR/message_definitions/v1.0/all.xml"
    ensure_mavgen_environment "$TARGET_DIR"
    validate_output_path "$OUTPUT_PATH" "$MAVGEN_LANGUAGE"
    run_mavgen "$TARGET_DIR" "$OUTPUT_PATH" "$MAVGEN_LANGUAGE"
}

main "$@"
