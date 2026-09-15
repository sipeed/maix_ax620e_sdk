#!/bin/sh
set -eu

# The board NCM userspace helper resolves ConfigFS through the vendor's g1
# name. Keep the sole lifecycle owner on that stable name even when it creates
# the ConfigFS gadget directly instead of calling the helper's RNDIS-capable
# start action.
DEFAULT_GADGET_NAME="g1"
VID="0x359f"
PID="0x4301"
BCD_USB="0x0200"
BCD_DEVICE="0x0200"
DEVICE_CLASS="0xEF"
DEVICE_SUBCLASS="0x02"
DEVICE_PROTOCOL="0x01"
UID_PATH="${STEREO_USB_UID_PATH:-/proc/ax_proc/uid}"
SERIAL=""
MANUFACTURER="Sipeed"
PRODUCT="Maix Stereo Depth Camera"
DEPTH_ROLE="Maix Stereo Depth Camera"
PSEUDO_COLOR_ROLE="Maix Stereo Pseudo Color Camera"
LEFT_IR_ROLE="Maix Stereo Left IR Camera"
RIGHT_IR_ROLE="Maix Stereo Right IR Camera"
CONFIGURATION="Raw Depth + Pseudo Color + Left IR + Right IR"
MAX_POWER="250"
LOCK_FILE="${STEREO_USB_LOCK_FILE:-/tmp/stereo-uvc-gadget.lock}"
STATE_ROOT="${STEREO_USB_STATE_ROOT:-/tmp}"
STATE_VERSION="9"
# Resolve helper files relative to the deployed script by default.  This keeps
# gadget and control operations on the same bundle when it is copied under
# /root for acceptance; deployments can still provide STEREO_USB_DEPLOY_DIR.
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
DEPLOY_DIR="${STEREO_USB_DEPLOY_DIR:-$SCRIPT_DIR}"
NETWORK_SCRIPT="${STEREO_USB_NETWORK_SCRIPT:-$DEPLOY_DIR/usb-gadget-network-unbound.sh}"
NETWORK_BRINGUP="${STEREO_USB_NETWORK_BRINGUP:-1}"
ENABLE_NCM="${STEREO_USB_ENABLE_NCM:-1}"
NCM_IFNAME="${STEREO_USB_NCM_IFNAME:-usb0}"
ROLE_SWITCH_PATH="${STEREO_USB_ROLE_SWITCH_PATH:-/sys/class/usb_role/8000000.dwc3-role-switch/role}"

# ConfigFS/UDC transitions are process-global. A second start/stop must never
# race the first one, because AXERA DWC3 can block in disconnect and leave the
# composite gadget deactivation count unbalanced.
exec 9>"$LOCK_FILE"
if ! flock -n 9; then
    echo 'Another gadget operation is running.' >&2
    exit 1
fi

case "$NETWORK_BRINGUP" in
    0|false|no)
        NETWORK_BRINGUP=0
        ;;
    1|true|yes)
        NETWORK_BRINGUP=1
        ;;
    *)
        echo 'STEREO_USB_NETWORK_BRINGUP must be 0/1, false/true, or no/yes.' >&2
        exit 2
        ;;
esac

case "$ENABLE_NCM" in
    0|false|no)
        ENABLE_NCM=0
        ;;
    1|true|yes)
        ENABLE_NCM=1
        ;;
    *)
        echo 'STEREO_USB_ENABLE_NCM must be 0/1, false/true, or no/yes.' >&2
        exit 2
        ;;
esac

find_configfs() {
    if [ -n "${STEREO_USB_CONFIGFS_ROOT:-}" ]; then
        printf '%s\n' "$STEREO_USB_CONFIGFS_ROOT"
        return
    fi
    # The AXERA NCM userspace helper resolves g1 through /etc/configfs. Mount
    # the same ConfigFS there so descriptor ownership and network bring-up see
    # one gadget tree. Other Linux platforms can fall back to the sysfs mount.
    mkdir -p /etc/configfs
    if ! mountpoint -q /etc/configfs 2>/dev/null; then
        mount -t configfs none /etc/configfs 2>/dev/null || true
    fi
    if [ -d /sys/kernel/config/usb_gadget ]; then
        printf '%s\n' /sys/kernel/config
        return
    fi
    if [ -d /etc/configfs/usb_gadget ]; then
        printf '%s\n' /etc/configfs
        return
    fi
    if [ -d /tmp/configfs/usb_gadget ]; then
        printf '%s\n' /tmp/configfs
        return
    fi
    echo 'ConfigFS usb_gadget subsystem is unavailable.' >&2
    return 1
}

CONFIGFS=""
GADGET_ROOT=""
GADGET=""
UDC_TO_BIND=""
CREATED_GADGET=0
LINK_MANIFEST=""
STATE_DIR=""

initialize_configfs() {
    CONFIGFS="$(find_configfs)"
    GADGET_ROOT="$CONFIGFS/usb_gadget"
}

resolve_serial() {
    if [ -n "${STEREO_USB_SERIAL:-}" ]; then
        case "$STEREO_USB_SERIAL" in
            MSD[0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F])
                SERIAL="$STEREO_USB_SERIAL"
                echo "Using USB serial override: $SERIAL" >&2
                return
                ;;
            *)
                echo 'STEREO_USB_SERIAL must be MSD plus 16 uppercase hex digits.' >&2
                exit 1
                ;;
        esac
    fi

    if [ ! -r "$UID_PATH" ]; then
        echo "Cannot read board UID: $UID_PATH" >&2
        exit 1
    fi
    uid_line="$(cat "$UID_PATH")"
    uid_hex="$(printf '%s\n' "$uid_line" | sed -n 's/^ax_uid:[[:space:]]*0x\([0-9a-fA-F]\{16\}\)[[:space:]]*$/\1/p')"
    if [ -z "$uid_hex" ]; then
        echo 'Invalid board UID format.' >&2
        exit 1
    fi
    uid_hex="$(printf '%s' "$uid_hex" | tr '[:lower:]' '[:upper:]')"
    if [ "$uid_hex" = "0000000000000000" ]; then
        echo 'Board UID is zero.' >&2
        exit 1
    fi
    SERIAL="MSD$uid_hex"
}

choose_gadget() {
    if [ -n "${STEREO_USB_GADGET_PATH:-}" ]; then
        GADGET="$STEREO_USB_GADGET_PATH"
        return
    fi
    if [ -n "${STEREO_UVC_GADGET_NAME:-}" ]; then
        GADGET="$GADGET_ROOT/$STEREO_UVC_GADGET_NAME"
        return
    fi
    existing_gadget=""
    existing_count=0
    for candidate in "$GADGET_ROOT"/*; do
        [ -d "$candidate" ] || continue
        existing_gadget="$candidate"
        existing_count=$((existing_count + 1))
        [ -f "$candidate/UDC" ] || continue
        current_udc="$(cat "$candidate/UDC" 2>/dev/null || true)"
        if [ -n "$current_udc" ]; then
            GADGET="$candidate"
            return
        fi
    done
    if [ -d "$GADGET_ROOT/g1" ]; then
        GADGET="$GADGET_ROOT/g1"
        return
    fi
    if [ "$existing_count" -eq 1 ]; then
        GADGET="$existing_gadget"
        return
    fi
    if [ "$existing_count" -gt 1 ]; then
        echo 'Multiple unbound gadgets found; set STEREO_UVC_GADGET_NAME.' >&2
        exit 1
    fi
    GADGET="$GADGET_ROOT/$DEFAULT_GADGET_NAME"
}

choose_udc() {
    current=""
    if [ -f "$GADGET/UDC" ]; then
        current="$(cat "$GADGET/UDC" 2>/dev/null || true)"
    fi
    if [ -n "$current" ]; then
        UDC_TO_BIND="$current"
    elif [ -n "${STEREO_UVC_UDC:-}" ]; then
        UDC_TO_BIND="$STEREO_UVC_UDC"
    else
        UDC_TO_BIND="$(ls /sys/class/udc 2>/dev/null | head -n 1)"
    fi
    if [ -z "$UDC_TO_BIND" ]; then
        echo 'No UDC found; set STEREO_UVC_UDC.' >&2
        exit 1
    fi
}

require_gadget_unbound() {
    if [ ! -e "$GADGET/UDC" ]; then
        echo "Missing UDC attribute: $GADGET/UDC" >&2
        return 1
    fi
    if [ ! -r "$GADGET/UDC" ]; then
        echo "Cannot read UDC: $GADGET/UDC" >&2
        return 1
    fi
    if ! bound_udc="$(cat "$GADGET/UDC" 2>/dev/null)"; then
        echo "Cannot read UDC: $GADGET/UDC" >&2
        return 1
    fi
    if [ -n "$bound_udc" ]; then
        echo "Gadget is still bound to $bound_udc." >&2
        return 1
    fi
    return 0
}

restore_binding() {
    if [ -n "$UDC_TO_BIND" ] && [ -d "$GADGET" ] && [ -f "$GADGET/UDC" ] &&
       [ -z "$(cat "$GADGET/UDC" 2>/dev/null || true)" ]; then
        printf '%s\n' "$UDC_TO_BIND" > "$GADGET/UDC" 2>/dev/null || true
    fi
}

state_dir_path() {
    printf '%s/stereo-uvc-state.%s\n' "$STATE_ROOT" "$(basename "$GADGET")"
}

snapshot_value() {
    key="$1"
    path="$2"
    if [ -f "$path" ]; then
        printf '%s\n' 1 > "$STATE_DIR/$key.present"
        cat "$path" > "$STATE_DIR/$key.value"
    else
        printf '%s\n' 0 > "$STATE_DIR/$key.present"
        : > "$STATE_DIR/$key.value"
    fi
}

restore_value() {
    key="$1"
    path="$2"
    if [ "$(cat "$STATE_DIR/$key.present" 2>/dev/null || true)" = 1 ]; then
        cat "$STATE_DIR/$key.value" > "$path"
    else
        rm -f "$path" 2>/dev/null || true
    fi
}

clear_descriptor_state() {
    [ -n "$STATE_DIR" ] || return 0
    [ -d "$STATE_DIR" ] || return 0
    rm -f "$STATE_DIR"/*.present "$STATE_DIR"/*.value \
        "$STATE_DIR/version" "$STATE_DIR/gadget_path" "$STATE_DIR/created" \
        "$STATE_DIR/device_strings_dir" "$STATE_DIR/config_strings_dir" \
        "$STATE_DIR/original_udc" "$STATE_DIR/links" \
        "$STATE_DIR/bind_attempted" "$STATE_DIR/bind_succeeded"
    rmdir "$STATE_DIR"
    STATE_DIR=""
}

remove_created_gadget() {
    rm -rf "$GADGET/configs/c.1" "$GADGET/strings/0x409" 2>/dev/null || true
    rmdir "$GADGET" 2>/dev/null || true
}

snapshot_descriptor_state() {
    STATE_DIR="$(state_dir_path)"
    if [ -e "$STATE_DIR" ]; then
        echo "Pending gadget state: $STATE_DIR" >&2
        return 1
    fi
    saved_umask="$(umask)"
    umask 077
    mkdir "$STATE_DIR"
    chmod 700 "$STATE_DIR"
    printf '%s\n' "$STATE_VERSION" > "$STATE_DIR/version"
    printf '%s\n' "$GADGET" > "$STATE_DIR/gadget_path"
    printf '%s\n' "$CREATED_GADGET" > "$STATE_DIR/created"
    cat "$GADGET/UDC" > "$STATE_DIR/original_udc"
    if [ "$CREATED_GADGET" -eq 1 ]; then
        umask "$saved_umask"
        return
    fi

    [ -d "$GADGET/strings/0x409" ] && printf '%s\n' 1 > "$STATE_DIR/device_strings_dir" ||
        printf '%s\n' 0 > "$STATE_DIR/device_strings_dir"
    [ -d "$GADGET/configs/c.1/strings/0x409" ] && printf '%s\n' 1 > "$STATE_DIR/config_strings_dir" ||
        printf '%s\n' 0 > "$STATE_DIR/config_strings_dir"
    snapshot_value idVendor "$GADGET/idVendor"
    snapshot_value idProduct "$GADGET/idProduct"
    snapshot_value bcdUSB "$GADGET/bcdUSB"
    snapshot_value bcdDevice "$GADGET/bcdDevice"
    snapshot_value bDeviceClass "$GADGET/bDeviceClass"
    snapshot_value bDeviceSubClass "$GADGET/bDeviceSubClass"
    snapshot_value bDeviceProtocol "$GADGET/bDeviceProtocol"
    snapshot_value serialnumber "$GADGET/strings/0x409/serialnumber"
    snapshot_value manufacturer "$GADGET/strings/0x409/manufacturer"
    snapshot_value product "$GADGET/strings/0x409/product"
    snapshot_value configuration "$GADGET/configs/c.1/strings/0x409/configuration"
    snapshot_value MaxPower "$GADGET/configs/c.1/MaxPower"
    : > "$STATE_DIR/links"
    for link_path in "$GADGET/configs/c.1"/*; do
        [ -L "$link_path" ] || continue
        link_name="$(basename "$link_path")"
        case "$link_name" in
            depth|depth_mjpeg|depth_raw|preview|pseudo_color|left_ir|right_ir) continue ;;
        esac
        printf '%s\t%s\n' "$link_name" "$(readlink "$link_path")" >> "$STATE_DIR/links"
    done
    umask "$saved_umask"
}

validate_descriptor_state() {
    STATE_DIR="$(state_dir_path)"
    [ -d "$STATE_DIR" ] || return 1
    [ "$(cat "$STATE_DIR/version" 2>/dev/null || true)" = "$STATE_VERSION" ] || return 1
    [ "$(cat "$STATE_DIR/gadget_path" 2>/dev/null || true)" = "$GADGET" ] || return 1
}

bind_attempt_was_recorded() {
    [ -e "$(state_dir_path)/bind_attempted" ]
}

bind_success_was_recorded() {
    [ -e "$(state_dir_path)/bind_succeeded" ]
}

record_bind_attempt() {
    if ! validate_descriptor_state; then
        echo 'Cannot record UDC bind state.' >&2
        return 1
    fi
    # Record the attempt before entering the kernel bind path.
    : > "$STATE_DIR/bind_attempted"
}

record_bind_success() {
    if ! validate_descriptor_state; then
        echo 'Cannot record successful UDC bind.' >&2
        return 1
    fi
    if [ "$(cat "$GADGET/UDC" 2>/dev/null || true)" != "$UDC_TO_BIND" ]; then
        echo "UDC bind verification failed: $UDC_TO_BIND" >&2
        return 1
    fi
    : > "$STATE_DIR/bind_succeeded"
}

require_no_failed_bind() {
    if bind_attempt_was_recorded && ! bind_success_was_recorded; then
        echo 'Previous UDC bind did not complete; state is read-only.' >&2
        echo 'Power-cycle the board before retrying.' >&2
        return 1
    fi
    return 0
}

migrate_legacy_success_state() {
    legacy_state="$(state_dir_path)"
    [ -d "$legacy_state" ] || return 1
    [ "$(cat "$legacy_state/version" 2>/dev/null || true)" = 8 ] || return 1
    [ "$(cat "$legacy_state/gadget_path" 2>/dev/null || true)" = "$GADGET" ] || return 1
    [ -e "$legacy_state/bind_attempted" ] || return 1
    [ -n "$(cat "$GADGET/UDC" 2>/dev/null || true)" ] || return 1
    uvc_configuration_ready || return 1
    printf '%s\n' "$STATE_VERSION" > "$legacy_state/version"
    : > "$legacy_state/bind_succeeded"
    STATE_DIR="$legacy_state"
}

restore_snapshotted_config_links() {
    [ -f "$STATE_DIR/links" ] || return 1
    mkdir -p "$GADGET/configs/c.1"
    for link_path in "$GADGET/configs/c.1"/*; do
        [ -L "$link_path" ] || continue
        link_name="$(basename "$link_path")"
        case "$link_name" in
            depth|depth_mjpeg|depth_raw|preview|pseudo_color|left_ir|right_ir) continue ;;
        esac
        rm -f "$link_path"
    done
    tab="$(printf '\t')"
    while IFS="$tab" read -r link_name link_target; do
        [ -n "$link_name" ] || continue
        (cd "$GADGET/configs/c.1" && ln -s "$link_target" "$link_name")
    done < "$STATE_DIR/links"
}

restore_descriptor_state() {
    validate_descriptor_state || {
        echo "Invalid gadget state: $(state_dir_path)" >&2
        return 1
    }
    if [ "$(cat "$STATE_DIR/created" 2>/dev/null || true)" = 1 ]; then
        clear_descriptor_state
        return
    fi
    original_udc="$(cat "$STATE_DIR/original_udc" 2>/dev/null || true)"
    restore_snapshotted_config_links
    mkdir -p "$GADGET/strings/0x409" "$GADGET/configs/c.1/strings/0x409"
    restore_value idVendor "$GADGET/idVendor"
    restore_value idProduct "$GADGET/idProduct"
    restore_value bcdUSB "$GADGET/bcdUSB"
    restore_value bcdDevice "$GADGET/bcdDevice"
    restore_value bDeviceClass "$GADGET/bDeviceClass"
    restore_value bDeviceSubClass "$GADGET/bDeviceSubClass"
    restore_value bDeviceProtocol "$GADGET/bDeviceProtocol"
    restore_value serialnumber "$GADGET/strings/0x409/serialnumber"
    restore_value manufacturer "$GADGET/strings/0x409/manufacturer"
    restore_value product "$GADGET/strings/0x409/product"
    restore_value configuration "$GADGET/configs/c.1/strings/0x409/configuration"
    restore_value MaxPower "$GADGET/configs/c.1/MaxPower"
    if [ "$(cat "$STATE_DIR/device_strings_dir" 2>/dev/null || true)" = 0 ]; then
        rmdir "$GADGET/strings/0x409" 2>/dev/null || true
    fi
    if [ "$(cat "$STATE_DIR/config_strings_dir" 2>/dev/null || true)" = 0 ]; then
        rmdir "$GADGET/configs/c.1/strings/0x409" 2>/dev/null || true
    fi
    clear_descriptor_state
    UDC_TO_BIND="$original_udc"
}

apply_descriptor_identity() {
    mkdir -p "$GADGET/strings/0x409" "$GADGET/configs/c.1/strings/0x409"
    printf '%s\n' "$VID" > "$GADGET/idVendor"
    printf '%s\n' "$PID" > "$GADGET/idProduct"
    printf '%s\n' "$BCD_USB" > "$GADGET/bcdUSB"
    printf '%s\n' "$BCD_DEVICE" > "$GADGET/bcdDevice"
    printf '%s\n' "$DEVICE_CLASS" > "$GADGET/bDeviceClass"
    printf '%s\n' "$DEVICE_SUBCLASS" > "$GADGET/bDeviceSubClass"
    printf '%s\n' "$DEVICE_PROTOCOL" > "$GADGET/bDeviceProtocol"
    printf '%s\n' "$SERIAL" > "$GADGET/strings/0x409/serialnumber"
    printf '%s\n' "$MANUFACTURER" > "$GADGET/strings/0x409/manufacturer"
    printf '%s\n' "$PRODUCT" > "$GADGET/strings/0x409/product"
    printf '%s\n' "$CONFIGURATION" > "$GADGET/configs/c.1/strings/0x409/configuration"
    printf '%s\n' "$MAX_POWER" > "$GADGET/configs/c.1/MaxPower"
}

restore_existing_config_links() {
    [ -n "$LINK_MANIFEST" ] && [ -f "$LINK_MANIFEST" ] || return 0
    tab="$(printf '\t')"
    while IFS="$tab" read -r link_name link_target; do
        [ -n "$link_name" ] || continue
        if [ ! -L "$GADGET/configs/c.1/$link_name" ]; then
            (cd "$GADGET/configs/c.1" && ln -s "$link_target" "$link_name")
        fi
    done < "$LINK_MANIFEST"
    rm -f "$LINK_MANIFEST"
    LINK_MANIFEST=""
}

stash_existing_config_links() {
    LINK_MANIFEST="/tmp/stereo-uvc-links.$$"
    : > "$LINK_MANIFEST"
    for link_path in "$GADGET/configs/c.1"/*; do
        [ -L "$link_path" ] || continue
        link_name="$(basename "$link_path")"
        case "$link_name" in
            depth|depth_mjpeg|depth_raw|preview|pseudo_color|left_ir|right_ir) continue ;;
        esac
        link_target="$(readlink "$link_path")"
        printf '%s\t%s\n' "$link_name" "$link_target" >> "$LINK_MANIFEST"
        rm -f "$link_path"
    done
}

discard_start_snapshot() {
    # This trap is armed only around the second unbound-state check. It may
    # discard task-owned /tmp metadata, but it must never touch ConfigFS: the
    # other process that won the race may already be entering kernel bind.
    if validate_descriptor_state; then
        clear_descriptor_state
    fi
}

bind_gadget() {
    record_bind_attempt || return 1
    if ! printf '%s\n' "$UDC_TO_BIND" > "$GADGET/UDC" 2>/dev/null; then
        echo "UDC bind failed: $UDC_TO_BIND" >&2
        return 1
    fi
    record_bind_success
}

config_value_is() {
    path="$1"
    expected="$2"
    [ -f "$path" ] && [ "$(cat "$path" 2>/dev/null || true)" = "$expected" ]
}

config_hex_value_is() {
    path="$1"
    expected="$2"
    [ -f "$path" ] || return 1
    actual="$(cat "$path" 2>/dev/null || true)"
    actual_lower="$(printf '%s' "$actual" | tr '[:upper:]' '[:lower:]')"
    expected_lower="$(printf '%s' "$expected" | tr '[:upper:]' '[:lower:]')"
    [ "$actual_lower" = "$expected_lower" ]
}

uvc_functions_present() {
    [ -d "$GADGET/functions/uvc.0" ] || [ -d "$GADGET/functions/uvc.1" ] ||
        [ -d "$GADGET/functions/uvc.2" ] || [ -d "$GADGET/functions/uvc.3" ]
}

uvc_function_topology_ready() {
    # ConfigFS normalizes hexadecimal attributes to lowercase on readback.
    config_hex_value_is "$GADGET/idVendor" "$VID" &&
    config_hex_value_is "$GADGET/idProduct" "$PID" &&
    config_hex_value_is "$GADGET/bcdUSB" "$BCD_USB" &&
    config_hex_value_is "$GADGET/bcdDevice" "$BCD_DEVICE" &&
    config_hex_value_is "$GADGET/bDeviceClass" "$DEVICE_CLASS" &&
    config_hex_value_is "$GADGET/bDeviceSubClass" "$DEVICE_SUBCLASS" &&
    config_hex_value_is "$GADGET/bDeviceProtocol" "$DEVICE_PROTOCOL" &&
    config_value_is "$GADGET/strings/0x409/serialnumber" "$SERIAL" &&
    config_value_is "$GADGET/strings/0x409/manufacturer" "$MANUFACTURER" &&
    config_value_is "$GADGET/strings/0x409/product" "$PRODUCT" &&
    config_value_is "$GADGET/configs/c.1/strings/0x409/configuration" "$CONFIGURATION" &&
    config_value_is "$GADGET/configs/c.1/MaxPower" "$MAX_POWER" &&
    [ -d "$GADGET/functions/uvc.0" ] &&
    [ -d "$GADGET/functions/uvc.1" ] &&
    [ -d "$GADGET/functions/uvc.2" ] &&
    [ -d "$GADGET/functions/uvc.3" ] &&
    [ ! -d "$GADGET/functions/uvc.4" ] &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/z16/1_depth/wWidth" 640 &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/z16/1_depth/wHeight" 384 &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/z16/1_depth/dwMaxVideoFrameBufferSize" 491520 &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/z16/1_depth/dwDefaultFrameInterval" 666666 &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/z16/1_depth/dwFrameInterval" 666666 &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/d16/1_depth/wWidth" 640 &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/d16/1_depth/wHeight" 384 &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/d16/1_depth/dwMaxVideoFrameBufferSize" 491520 &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/d16/1_depth/dwDefaultFrameInterval" 666666 &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/d16/1_depth/dwFrameInterval" 666666 &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/y16/1_depth/wWidth" 640 &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/y16/1_depth/wHeight" 384 &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/y16/1_depth/dwMaxVideoFrameBufferSize" 491520 &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/y16/1_depth/dwDefaultFrameInterval" 666666 &&
    config_value_is "$GADGET/functions/uvc.0/streaming/uncompressed/y16/1_depth/dwFrameInterval" 666666 &&
    config_value_is "$GADGET/functions/uvc.0/streaming_maxpacket" 1024 &&
    config_value_is "$GADGET/functions/uvc.0/streaming_mult" 0 &&
    config_value_is "$GADGET/functions/uvc.0/streaming_bulk" 0 &&
    # ConfigFS creates the empty mjpeg container for every UVC function;
    # reject only an actual legacy Raw-node MJPEG frame declaration.
    [ ! -d "$GADGET/functions/uvc.0/streaming/mjpeg/m/1_depth_turbo" ] &&
    config_value_is "$GADGET/functions/uvc.1/streaming/mjpeg/m/1_pseudo_color/wWidth" 640 &&
    config_value_is "$GADGET/functions/uvc.1/streaming/mjpeg/m/1_pseudo_color/wHeight" 384 &&
    config_value_is "$GADGET/functions/uvc.1/streaming/mjpeg/m/1_pseudo_color/dwDefaultFrameInterval" 666666 &&
    config_value_is "$GADGET/functions/uvc.1/streaming/mjpeg/m/1_pseudo_color/dwFrameInterval" 666666 &&
    config_value_is "$GADGET/functions/uvc.1/streaming_maxpacket" 512 &&
    config_value_is "$GADGET/functions/uvc.1/streaming_mult" 0 &&
    config_value_is "$GADGET/functions/uvc.1/streaming_bulk" 0 &&
    config_value_is "$GADGET/functions/uvc.2/streaming/mjpeg/m/1_ir/wWidth" 640 &&
    config_value_is "$GADGET/functions/uvc.2/streaming/mjpeg/m/1_ir/wHeight" 480 &&
    config_value_is "$GADGET/functions/uvc.2/streaming/mjpeg/m/1_ir/dwDefaultFrameInterval" 333333 &&
    config_value_is "$GADGET/functions/uvc.2/streaming/mjpeg/m/1_ir/dwFrameInterval" 333333 &&
    config_value_is "$GADGET/functions/uvc.2/streaming_maxpacket" 512 &&
    config_value_is "$GADGET/functions/uvc.2/streaming_mult" 0 &&
    config_value_is "$GADGET/functions/uvc.2/streaming_bulk" 0 &&
    config_value_is "$GADGET/functions/uvc.3/streaming/mjpeg/m/1_ir/wWidth" 640 &&
    config_value_is "$GADGET/functions/uvc.3/streaming/mjpeg/m/1_ir/wHeight" 480 &&
    config_value_is "$GADGET/functions/uvc.3/streaming/mjpeg/m/1_ir/dwDefaultFrameInterval" 333333 &&
    config_value_is "$GADGET/functions/uvc.3/streaming/mjpeg/m/1_ir/dwFrameInterval" 333333 &&
    config_value_is "$GADGET/functions/uvc.3/streaming_maxpacket" 512 &&
    config_value_is "$GADGET/functions/uvc.3/streaming_bulk" 0
}

uvc_config_links_ready() {
    [ -L "$GADGET/configs/c.1/depth" ] &&
    [ -L "$GADGET/configs/c.1/pseudo_color" ] &&
    [ -L "$GADGET/configs/c.1/left_ir" ] &&
    [ -L "$GADGET/configs/c.1/right_ir" ] &&
    # ConfigFS normalizes relative link targets to a longer path on readback;
    # compare canonical targets rather than the literal ln -s argument.
    [ "$(readlink -f "$GADGET/configs/c.1/depth" 2>/dev/null || true)" = "$GADGET/functions/uvc.0" ] &&
    [ "$(readlink -f "$GADGET/configs/c.1/pseudo_color" 2>/dev/null || true)" = "$GADGET/functions/uvc.1" ] &&
    [ "$(readlink -f "$GADGET/configs/c.1/left_ir" 2>/dev/null || true)" = "$GADGET/functions/uvc.2" ] &&
    [ "$(readlink -f "$GADGET/configs/c.1/right_ir" 2>/dev/null || true)" = "$GADGET/functions/uvc.3" ]
}

uvc_configuration_ready() {
    uvc_function_topology_ready && uvc_config_links_ready
}

write_guid_z16() {
    printf '\x5a\x31\x36\x20\x00\x00\x10\x00\x80\x00\x00\xaa\x00\x38\x9b\x71'
}

write_guid_y16() {
    printf '\x59\x31\x36\x20\x00\x00\x10\x00\x80\x00\x00\xaa\x00\x38\x9b\x71'
}

write_guid_d16() {
    printf '\x50\x00\x00\x00\x04\x00\x10\x00\x80\x00\x00\xaa\x00\x38\x9b\x71'
}

create_uncompressed_depth_format() {
    function_name="$1"
    format_name="$2"
    guid_writer="$3"
    format_dir="$GADGET/functions/$function_name/streaming/uncompressed/$format_name"
    frame_dir="$format_dir/1_depth"
    mkdir -p "$frame_dir"
    "$guid_writer" > "$format_dir/guidFormat"
    printf '%s\n' 16 > "$format_dir/bBitsPerPixel"
    printf '%s\n' 1 > "$format_dir/bDefaultFrameIndex"
    printf '%s\n' 640 > "$frame_dir/wWidth"
    printf '%s\n' 384 > "$frame_dir/wHeight"
    printf '%s\n' 491520 > "$frame_dir/dwMaxVideoFrameBufferSize"
    printf '%s\n' 58982400 > "$frame_dir/dwMinBitRate"
    printf '%s\n' 58982400 > "$frame_dir/dwMaxBitRate"
    printf '%s\n' 666666 > "$frame_dir/dwDefaultFrameInterval"
    printf '%s\n' 666666 > "$frame_dir/dwFrameInterval"
}

create_mjpeg_frame() {
    function_name="$1"
    frame_name="$2"
    width="$3"
    height="$4"
    interval="$5"
    fps="$6"
    frame_dir="$GADGET/functions/$function_name/streaming/mjpeg/m/$frame_name"
    max_frame=1048576
    bit_rate=$((max_frame * 8 * fps))
    mkdir -p "$frame_dir"
    printf '%s\n' "$width" > "$frame_dir/wWidth"
    printf '%s\n' "$height" > "$frame_dir/wHeight"
    printf '%s\n' "$max_frame" > "$frame_dir/dwMaxVideoFrameBufferSize"
    printf '%s\n' "$bit_rate" > "$frame_dir/dwMinBitRate"
    printf '%s\n' "$bit_rate" > "$frame_dir/dwMaxBitRate"
    printf '%s\n' "$interval" > "$frame_dir/dwDefaultFrameInterval"
    printf '%s\n' "$interval" > "$frame_dir/dwFrameInterval"
}

link_uvc_headers() {
    function_name="$1"
    maxpacket="$2"
    shift 2
    header_dir="$GADGET/functions/$function_name/streaming/header/h"
    mkdir -p "$header_dir"
    for target in "$@"; do
        # AXERA 4.19 configfs resolves a relative target against ln's cwd;
        # follow the SDK sample and create links from inside the owning group.
        (cd "$header_dir" && ln -s "$target")
    done
    for speed in fs hs ss; do
        class_dir="$GADGET/functions/$function_name/streaming/class/$speed"
        mkdir -p "$class_dir"
        (cd "$class_dir" && ln -s ../../header/h)
    done
    control="$GADGET/functions/$function_name/control"
    mkdir -p "$control/header/h"
    mkdir -p "$control/class/fs" "$control/class/ss"
    (cd "$control/class/fs" && ln -s ../../header/h)
    (cd "$control/class/ss" && ln -s ../../header/h)
    # AXERA Linux 4.19 bulk mode can enter UVC_STATE_STREAMING during Host
    # enumeration before userspace queues buffers, making VIDIOC_STREAMON fail
    # with ENODEV. Use the SDK-supported high-speed isochronous path instead.
    # The AX630C DWC3 reports eight IN endpoints, but its TX FIFO layout gives
    # only ep1in/ep2in enough capacity for a 1024-byte packet; later IN
    # endpoints top out at 524 bytes. Keep the combined Depth function first
    # at 1024 for 7.37 MB/s Raw16, then use 512 for each MJPEG IR stream.
    printf '%s\n' "$maxpacket" > "$GADGET/functions/$function_name/streaming_maxpacket"
    printf '%s\n' 1 > "$GADGET/functions/$function_name/streaming_interval"
    printf '%s\n' 15 > "$GADGET/functions/$function_name/streaming_maxburst"
    printf '%s\n' 0 > "$GADGET/functions/$function_name/streaming_mult"
    printf '%s\n' 0 > "$GADGET/functions/$function_name/streaming_bulk"
}

create_uvc_functions() {
    mkdir -p "$GADGET/functions/uvc.0"
    create_uncompressed_depth_format uvc.0 z16 write_guid_z16
    create_uncompressed_depth_format uvc.0 d16 write_guid_d16
    create_uncompressed_depth_format uvc.0 y16 write_guid_y16
    link_uvc_headers uvc.0 1024 ../../uncompressed/z16 ../../uncompressed/d16 ../../uncompressed/y16

    mkdir -p "$GADGET/functions/uvc.1"
    create_mjpeg_frame uvc.1 1_pseudo_color 640 384 666666 15
    link_uvc_headers uvc.1 512 ../../mjpeg/m

    mkdir -p "$GADGET/functions/uvc.2"
    create_mjpeg_frame uvc.2 1_ir 640 480 333333 30
    link_uvc_headers uvc.2 512 ../../mjpeg/m
    mkdir -p "$GADGET/functions/uvc.3"
    create_mjpeg_frame uvc.3 1_ir 640 480 333333 30
    link_uvc_headers uvc.3 512 ../../mjpeg/m

    link_uvc_config_functions
}

link_uvc_config_functions() {
    # ConfigFS binds functions in config-link creation order. Put UVC first so
    # NCM does not consume the DWC3 endpoints that are also ISO-capable.
    stash_existing_config_links
    (cd "$GADGET/configs/c.1" && ln -s ../../functions/uvc.0 depth)
    (cd "$GADGET/configs/c.1" && ln -s ../../functions/uvc.1 pseudo_color)
    (cd "$GADGET/configs/c.1" && ln -s ../../functions/uvc.2 left_ir)
    (cd "$GADGET/configs/c.1" && ln -s ../../functions/uvc.3 right_ir)
    restore_existing_config_links
}

remove_configuration_links() {
    # start attaches depth, pseudo-color, left IR, right IR, then NCM. Detach
    # their configuration links in the exact reverse order. Keep every UVC
    # function and its internal graph intact for the next start.
    rm -f "$GADGET/os_desc/c.1"
    rm -f "$GADGET/configs/c.1/ncm.usb1"
    rm -f "$GADGET/configs/c.1/right_ir"
    rm -f "$GADGET/configs/c.1/left_ir"
    rm -f "$GADGET/configs/c.1/pseudo_color"
    rm -f "$GADGET/configs/c.1/depth"

    # Rejecting an unrelated pre-existing configuration would leave the gadget
    # half-stopped. Detach any remaining top-level function links.
    for link_path in "$GADGET/configs/c.1"/*; do
        [ -L "$link_path" ] || continue
        rm -f "$link_path"
    done
}

ncm_config_ready() {
    [ -d "$GADGET/functions/ncm.usb1" ] &&
    [ -L "$GADGET/configs/c.1/ncm.usb1" ] &&
    [ "$(readlink -f "$GADGET/configs/c.1/ncm.usb1" 2>/dev/null || true)" = \
      "$GADGET/functions/ncm.usb1" ]
}

select_device_role() {
    [ -e "$ROLE_SWITCH_PATH" ] || return 0
    if ! printf '%s\n' device > "$ROLE_SWITCH_PATH" 2>/dev/null; then
        echo "USB role switch failed: $ROLE_SWITCH_PATH" >&2
        return 1
    fi
}

remove_rndis_unbound() {
    if [ -L "$GADGET/configs/c.1/rndis.usb0" ]; then
        rm "$GADGET/configs/c.1/rndis.usb0" || return 1
    fi
    if [ -d "$GADGET/functions/rndis.usb0" ]; then
        rmdir "$GADGET/functions/rndis.usb0" || return 1
    fi
}

configure_ncm_os_descriptor() {
    mkdir -p "$GADGET/os_desc"
    printf '%s\n' WINNCM > "$GADGET/functions/ncm.usb1/os_desc/interface.ncm/compatible_id"
    printf '%s\n' 1 > "$GADGET/os_desc/use"
    printf '%s\n' 0xCD > "$GADGET/os_desc/b_vendor_code"
    printf '%s\n' MSFT100 > "$GADGET/os_desc/qw_sign"
    if [ -e "$GADGET/os_desc/c.1" ] && [ ! -L "$GADGET/os_desc/c.1" ]; then
        echo "Invalid NCM OS descriptor link: $GADGET/os_desc/c.1" >&2
        return 1
    fi
    rm -f "$GADGET/os_desc/c.1"
    (cd "$GADGET/os_desc" && ln -s ../configs/c.1 c.1)
}

prepare_network_unbound() {
    if [ ! -d "$GADGET" ]; then
        mkdir -p "$GADGET_ROOT"
        mkdir "$GADGET"
        CREATED_GADGET=1
    fi
    if [ ! -r "$GADGET/UDC" ] ||
       [ -n "$(cat "$GADGET/UDC" 2>/dev/null || true)" ]; then
        echo 'NCM setup requires an unbound gadget.' >&2
        return 1
    fi

    # Do not call the board helper's `start`: it sources /boot/configs and can
    # create RNDIS before this script gets a chance to remove it. Four UVC
    # streaming endpoints plus NCM use seven of AX630C's eight endpoints.
    remove_rndis_unbound
    if [ "$ENABLE_NCM" -eq 0 ]; then
        rm -f "$GADGET/configs/c.1/ncm.usb1"
        if [ -d "$GADGET/functions/ncm.usb1" ]; then
            rmdir "$GADGET/functions/ncm.usb1" || return 1
        fi
        return 0
    fi
    mkdir -p "$GADGET/configs/c.1" "$GADGET/functions/ncm.usb1"
    if [ -e "$GADGET/configs/c.1/ncm.usb1" ] &&
       [ ! -L "$GADGET/configs/c.1/ncm.usb1" ]; then
        echo "Invalid NCM config link: $GADGET/configs/c.1/ncm.usb1" >&2
        return 1
    fi
    if [ -L "$GADGET/configs/c.1/ncm.usb1" ] && ! ncm_config_ready; then
        rm "$GADGET/configs/c.1/ncm.usb1" || return 1
    fi
    if [ ! -L "$GADGET/configs/c.1/ncm.usb1" ]; then
        (cd "$GADGET/configs/c.1" && ln -s ../../functions/ncm.usb1 ncm.usb1)
    fi
    configure_ncm_os_descriptor
    if ! ncm_config_ready; then
        echo 'NCM function or config link is missing.' >&2
        return 1
    fi
}

start_network_after_bind() {
    [ "$ENABLE_NCM" -eq 1 ] && [ "$NETWORK_BRINGUP" -eq 1 ] || return 0
    if ! ip link show dev "$NCM_IFNAME" >/dev/null 2>&1; then
        echo "NCM interface not found: $NCM_IFNAME" >&2
        return 0
    fi
    if ip -4 addr show dev "$NCM_IFNAME" 2>/dev/null |
       grep -q '[[:space:]]inet[[:space:]]'; then
        return 0
    fi
    if ! STEREO_USB_NCM_IFNAME="$NCM_IFNAME" "$NETWORK_SCRIPT" ncm_start \
         9>&- >/dev/null 2>&1; then
        echo "NCM start failed: $NCM_IFNAME" >&2
    fi
}

stop_network_before_unbind() {
    [ "$ENABLE_NCM" -eq 1 ] && [ "$NETWORK_BRINGUP" -eq 1 ] || return 0
    # The interface may already be gone after a host disconnect.  Network
    # teardown is best-effort and must never prevent UDC unbind; leaving the
    # gadget bound here makes the next start observe a stale configuration.
    if ! ip link show dev "$NCM_IFNAME" >/dev/null 2>&1; then
        return 0
    fi
    if ! STEREO_USB_NCM_IFNAME="$NCM_IFNAME" "$NETWORK_SCRIPT" ncm_stop \
         9>&- >/dev/null 2>&1; then
        echo "NCM stop failed: $NCM_IFNAME" >&2
        return 0
    fi
}

stop_gadget() {
    initialize_configfs
    choose_gadget
    if [ ! -d "$GADGET" ]; then
        return
    fi

    if [ ! -r "$GADGET/UDC" ]; then
        echo "Cannot read UDC: $GADGET/UDC" >&2
        return 1
    fi
    if ! current_udc="$(cat "$GADGET/UDC" 2>/dev/null)"; then
        echo "Cannot read UDC: $GADGET/UDC" >&2
        return 1
    fi

    if [ -z "$current_udc" ]; then
        return
    fi

    # Stop networking, unbind, then detach only top-level configuration links.
    # AXERA 4.19 crashes in uvcg_streaming_class_drop_link when UVC-internal
    # links are removed after a runtime unbind, so function teardown is unsafe.
    stop_network_before_unbind
    if ! printf '\n' > "$GADGET/UDC" 2>/dev/null; then
        echo "UDC unbind failed: $current_udc" >&2
        return 1
    fi
    remove_configuration_links

    STATE_DIR="$(state_dir_path)"
    if [ -d "$STATE_DIR" ]; then
        clear_descriptor_state
    fi
    UDC_TO_BIND=""
}

start_gadget() {
    resolve_serial
    initialize_configfs
    choose_gadget

    current_udc=""
    if [ -f "$GADGET/UDC" ]; then
        current_udc="$(cat "$GADGET/UDC" 2>/dev/null || true)"
    fi
    if [ -n "$current_udc" ] && uvc_configuration_ready; then
        if ! { [ "${ENABLE_NCM:-1}" -eq 0 ] || ncm_config_ready; } ||
           [ -L "$GADGET/configs/c.1/rndis.usb0" ] ||
           [ -d "$GADGET/functions/rndis.usb0" ]; then
            echo 'Bound gadget network configuration does not match.' >&2
            return 1
        fi
        if ! bind_success_was_recorded; then
            migrate_legacy_success_state || true
        fi
        require_no_failed_bind
        if ! validate_descriptor_state || ! bind_success_was_recorded; then
            echo 'Bound gadget is missing managed state.' >&2
            return 1
        fi
        echo "Gadget already started on $current_udc."
        start_network_after_bind
        return 0
    fi
    if [ -n "$current_udc" ] && uvc_functions_present; then
        echo 'Bound gadget configuration does not match.' >&2
        return 1
    fi

    CREATED_GADGET=0
    require_no_failed_bind
    select_device_role
    prepare_network_unbound

    # ConfigFS topology is modified only while the gadget is unbound.
    require_gadget_unbound
    choose_udc
    snapshot_descriptor_state
    trap discard_start_snapshot EXIT INT TERM
    require_gadget_unbound
    # Once ConfigFS mutation starts, any failure must preserve the resulting
    # topology. This makes descriptor creation failures available for
    # postmortem instead of hiding the cause.
    trap - EXIT INT TERM
    mkdir -p "$GADGET/configs/c.1"
    if uvc_function_topology_ready; then
        link_uvc_config_functions
    elif uvc_functions_present; then
        echo 'Existing UVC function graph is incomplete or has stale descriptors; power-cycle before retrying.' >&2
        return 1
    else
        apply_descriptor_identity
        create_uvc_functions
    fi
    if ! uvc_configuration_ready ||
       ! { [ "${ENABLE_NCM:-1}" -eq 0 ] || ncm_config_ready; }; then
        echo 'Gadget configuration is incomplete.' >&2
        return 1
    fi
    if ! bind_gadget; then
        echo 'UDC bind failed; state preserved.' >&2
        echo 'Power-cycle the board before retrying.' >&2
        return 1
    fi
    echo "Gadget started: $(basename "$GADGET") -> $UDC_TO_BIND (serial=$SERIAL)."
    UDC_TO_BIND=""
    start_network_after_bind
}

restart_gadget() {
    stop_gadget
    start_gadget
}

case "${1:-}" in
    start)
        start_gadget
        ;;
    stop)
        stop_gadget
        echo 'Gadget stopped.'
        ;;
    restart)
        restart_gadget
        ;;
    serial)
        resolve_serial
        printf '%s\n' "$SERIAL"
        ;;
    *)
        echo "Usage: $0 start|stop|restart|serial" >&2
        exit 2
        ;;
esac
