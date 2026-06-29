#!/bin/bash

###
### NanoKVM WiFi Manager — Wireless Network Connection and Hotspot Management Tool
###
### Usage:
###   $0 [command] [parameters]
###
### Commands:
### start                            Load driver and start wpa_supplicant (system service entry)
### stop                             Stop all WiFi services and unload driver (system service exit)
### on                               Turn on WiFi and reconnect to last saved network
### off                              Turn off WiFi (disconnect, keep saved networks)
### connect <SSID> [Password]        Connect to specified WiFi and save configuration
### enterprise <SSID> <PEAP|TLS|TTLS> <identity> [options...]
###                                  Connect to enterprise WiFi (EAP)
### status                           Show current WiFi status
### signal                           Get current WiFi signal strength (dBm)
### scan                             Scan for available WiFi networks (JSON)
### saved                            List saved WiFi networks (JSON)
### remove <SSID>                    Remove a saved WiFi network
### clear                            Clear all saved WiFi networks
### ap <SSID> [Password]             Start WiFi hotspot
### ap_off                           Stop WiFi hotspot
### device_count                     Get AP-connected device count
### ap_ip                            Get AP gateway IP address
###
### Options:
###   -h, --help        Display this help information
###   -d, --debug      Enable debug mode (show detailed logs)
###
### Configuration:
###   Temporary Directory: /dev/shm/tmp/wifi
###   Driver Type: nl80211
###   Ethernet Interface: eth0
###   DHCP Client: systemd udhcpc@<iface>.service
### Persistent Configuration: /etc/kvm/wifi.conf, /etc/wpa_supplicant.conf
###
### Example:
###   $0 on                              # Turn on and auto-reconnect
###   $0 off                             # Disconnect
###   $0 connect MyWiFi 12345678         # Connect and save
###   $0 status                          # Show status
###   $0 signal                          # Get signal strength
###   $0 ap MyHotspot mypassword         # Create a hotspot
###

DEBUG="false"
for arg in "$@"; do
    if [[ "$arg" == "-d" || "$arg" == "--debug" ]]; then
        DEBUG="true"
        break
    fi
done

# Configuration Constants
readonly TMP_DIR="/dev/shm/tmp/wifi"
readonly WPA_CONF_FILE="${WPA_CONF_FILE:-/etc/wpa_supplicant.conf}"
readonly WPA_PID_FILE="${TMP_DIR}/wpa.pid"
readonly DRIVER="nl80211"
readonly ETH_IFACE="eth0"
readonly SUBNET="192.168.12"
readonly GATEWAY="$SUBNET.1"
readonly HOSTAPD_CONF="$TMP_DIR/hostapd.conf"
readonly UDHCPD_CONF="$TMP_DIR/udhcpd.wlan0.conf"
readonly WIFI_CFG_PATH="/etc/kvm/wifi.conf"
readonly HW_WIFI_CFG_PATH="/etc/kvm/hw/wifi"
readonly LOCK_FILE="${TMP_DIR}/wifi.lock"
readonly LOCK_FD=200
readonly LOCK_WAIT_SECONDS=30
readonly PREVIOUS_WIFI_SAVE="/etc/kvm/wifi_save"
readonly PREVIOUS_WIFI="$TMP_DIR/previous_wifi"
readonly LAST_WIFI_FILE="/etc/kvm/last_wifi"
readonly AP_PREVIOUS_STATE_FILE="/etc/kvm/wifi_before_ap.conf"

# Conditionally set locale to avoid warnings on systems without en_US.UTF-8
if locale -a 2>/dev/null | grep -q "en_US.UTF-8"; then
    export LANG=en_US.UTF-8
    export LC_ALL=en_US.UTF-8
    export LC_CTYPE=en_US.UTF-8
else
    export LC_ALL=C
    export LANG=C
    export LC_CTYPE=C
fi

# set -x

debug_print() {
    [[ "${DEBUG}" == "true" ]] && printf "[DEBUG] %s\n" "$*"
}

show_help() {
    sed -rn 's/^### ?//;T;p' "$0" | awk '
        BEGIN { in_example = 0 }
        /^EXAMPLE: / { in_example = 1 }
        in_example && /^###   \S/ { sub("###   ", "  "); print }
        !in_example { print }
    '
}

# Ensure temporary directory exists
mkdir -p "${TMP_DIR}" || {
    debug_print "[wifi] failed to create temporary directory: ${TMP_DIR}" >&2
    exit 1
}

# Ensure persistent config directories exist
mkdir -p "/etc/kvm" "/etc/kvm/hw" || {
    debug_print "[wifi] failed to create persistent config directories under /etc/kvm" >&2
    exit 1
}

acquire_lock() {
    if ! command -v flock >/dev/null 2>&1; then
        echo "[wifi] flock command not found, cannot acquire lock" >&2
        exit 1
    fi

    exec 200>"$LOCK_FILE"

    local rc=0
    flock -n "$LOCK_FD"
    rc=$?
    if [[ $rc -eq 0 ]]; then
        return 0
    fi
    if [[ $rc -ne 1 ]]; then
        echo "[wifi] failed to acquire lock: flock returned ${rc}" >&2
        exit 1
    fi

    debug_print "[wifi] another instance is running, waiting for lock..."
    local attempt=0
    for ((attempt = 1; attempt <= LOCK_WAIT_SECONDS; attempt++)); do
        flock -n "$LOCK_FD"
        rc=$?
        if [[ $rc -eq 0 ]]; then
            debug_print "[wifi] lock acquired after ${attempt}s wait"
            return 0
        fi
        if [[ $rc -ne 1 ]]; then
            echo "[wifi] failed while waiting for lock: flock returned ${rc}" >&2
            exit 1
        fi
        sleep 1
    done

    echo "[wifi] failed to acquire lock after ${LOCK_WAIT_SECONDS} seconds" >&2
    exit 1
}

release_lock() {
    flock -u "$LOCK_FD"
}

trap release_lock EXIT INT TERM

acquire_lock

validate_arguments() {
    local expected=$1
    local actual=$2
    ((actual >= expected)) || {
        debug_print "[wifi] invalid argument count. Expected: ${expected}, Received: ${actual}" >&2
        exit 1
    }
}

get_wireless_interface() {
    local interfaces
    interfaces=$(ip -o link show | grep -o 'wlan[^:]*' | head -1)

    if [[ -z "${interfaces}" ]]; then
        # debug_print "No available wireless interfaces detected" >&2
        return 1
    fi

    echo "${interfaces}"
}

ensure_wpa_conf() {
    if [[ -s "$WPA_CONF_FILE" ]]; then
        # Ensure ctrl_interface uses /var/run for consistency with wifi.sh
        if grep -q '^ctrl_interface=/run/wpa_supplicant' "$WPA_CONF_FILE"; then
            sed -i 's|^ctrl_interface=/run/wpa_supplicant|ctrl_interface=/var/run/wpa_supplicant|' "$WPA_CONF_FILE"
            debug_print "[wifi] updated ctrl_interface path in $WPA_CONF_FILE"
        fi
        return 0
    fi
    if [[ ! -s "$WPA_CONF_FILE" ]]; then
        mkdir -p "$(dirname "$WPA_CONF_FILE")"
        cat <<EOF >"$WPA_CONF_FILE"
ctrl_interface=/var/run/wpa_supplicant
update_config=1
EOF
        debug_print "[wifi] created default wpa config at $WPA_CONF_FILE"
    fi
}

mark_saved_networks_disabled() {
    if [[ ! -f "$WPA_CONF_FILE" ]]; then
        return 0
    fi

    local tmp_file="${WPA_CONF_FILE}.tmp"
    if ! awk '
        BEGIN { in_network = 0; has_disabled = 0 }
        /^[[:space:]]*network[[:space:]]*=\{/ {
            in_network = 1
            has_disabled = 0
            print
            next
        }
        in_network && /^[[:space:]]*disabled[[:space:]]*=/ {
            if (!has_disabled) {
                print "        disabled=1"
                has_disabled = 1
            }
            next
        }
        in_network && /^[[:space:]]*\}/ {
            if (!has_disabled) {
                print "        disabled=1"
            }
            in_network = 0
            has_disabled = 0
            print
            next
        }
        { print }
    ' "$WPA_CONF_FILE" >"$tmp_file"; then
        rm -f "$tmp_file" || true
        debug_print "[wifi] failed to mark saved networks disabled in $WPA_CONF_FILE" >&2
        return 1
    fi

    mv "$tmp_file" "$WPA_CONF_FILE"
    debug_print "[wifi] marked all saved networks disabled in $WPA_CONF_FILE"
    return 0
}

write_wifi_boot_config() {
    local ssid="$1"
    [[ -n "$ssid" ]] || return 0
    cat <<EOF >"$WIFI_CFG_PATH"
WIFI_CFG_SSID=${ssid}
EOF
}

clear_wifi_boot_config() {
    rm -f "$WIFI_CFG_PATH" || true
}

read_current_sta_ssid() {
    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface) || return 1

    if [[ ! -S "/var/run/wpa_supplicant/${WIFI_IFACE}" ]]; then
        return 1
    fi

    local state
    state=$(wpa_cli -i "$WIFI_IFACE" status 2>/dev/null | grep "^wpa_state=" | cut -d= -f2)
    if [[ "$state" != "COMPLETED" ]]; then
        return 1
    fi

    local ssid
    ssid=$(wpa_cli -i "$WIFI_IFACE" status 2>/dev/null | grep "^ssid=" | cut -d= -f2-)
    [[ -n "$ssid" ]] || return 1

    echo "$ssid"
    return 0
}

save_ap_previous_state() {
    local ssid
    ssid=$(read_current_sta_ssid 2>/dev/null || true)

    if [[ -n "$ssid" ]]; then
        cat <<EOF >"$AP_PREVIOUS_STATE_FILE"
AP_PREVIOUS_STATE=CONNECTED
AP_PREVIOUS_SSID=${ssid}
EOF
        debug_print "[wifi] saved AP previous state: CONNECTED ${ssid}"
        return 0
    fi

    cat <<EOF >"$AP_PREVIOUS_STATE_FILE"
AP_PREVIOUS_STATE=DISABLED
AP_PREVIOUS_SSID=
EOF
    debug_print "[wifi] saved AP previous state: DISABLED"
    return 0
}

clear_ap_previous_state() {
    rm -f "$AP_PREVIOUS_STATE_FILE" || true
}

resolve_boot_config_action_from_ap_previous_state() {
    if [[ ! -f "$AP_PREVIOUS_STATE_FILE" ]]; then
        echo "clear"
        return 0
    fi

    local prev_state
    prev_state=$(grep -E '^AP_PREVIOUS_STATE=' "$AP_PREVIOUS_STATE_FILE" | cut -d= -f2-)
    local prev_ssid
    prev_ssid=$(grep -E '^AP_PREVIOUS_SSID=' "$AP_PREVIOUS_STATE_FILE" | cut -d= -f2-)

    if [[ "$prev_state" == "CONNECTED" && -n "$prev_ssid" ]]; then
        echo "keep-connected|${prev_ssid}"
        return 0
    fi

    echo "clear"
    return 0
}

is_ap_runtime_active() {
    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface 2>/dev/null || true)

    if pgrep hostapd >/dev/null 2>&1; then
        return 0
    fi

    if [[ -n "$WIFI_IFACE" ]] && ip -4 addr show dev "$WIFI_IFACE" 2>/dev/null | grep -qE "inet ${GATEWAY}/24"; then
        return 0
    fi

    if [[ -f "$AP_PREVIOUS_STATE_FILE" ]]; then
        return 0
    fi

    return 1
}

restore_boot_config_from_ap_previous_state() {
    local decision
    decision=$(resolve_boot_config_action_from_ap_previous_state)

    if [[ "$decision" == keep-connected\|* ]]; then
        local prev_ssid
        prev_ssid=${decision#keep-connected|}
        write_wifi_boot_config "$prev_ssid"
        debug_print "[wifi] restored boot config from AP previous state: ${prev_ssid}"
    else
        clear_wifi_boot_config
        debug_print "[wifi] AP previous state not connected, cleared boot config"
    fi

    clear_ap_previous_state
    return 0
}

start_dhcp_client() {
    local iface="$1"
    if systemctl start "udhcpc@${iface}.service" >/dev/null 2>&1; then
        debug_print "[wifi] started udhcpc@${iface}.service"
        local dhcp_retry=0
        while [ $dhcp_retry -lt 20 ]; do
            if ip addr show "$iface" 2>/dev/null | grep -qE 'inet '; then
                debug_print "[wifi] IP address acquired on $iface"
                return 0
            fi
            sleep 0.5
            ((dhcp_retry++))
        done
        debug_print "[wifi] DHCP timeout on $iface" >&2
        return 1
    fi
    debug_print "[wifi] systemd udhcpc service unavailable, falling back to direct udhcpc"
    if udhcpc -i "$iface" -s /usr/share/udhcpc/default.script >/dev/null 2>&1; then
        return 0
    fi
    debug_print "[wifi] direct udhcpc failed on $iface" >&2
    return 1
}

stop_dhcp_client() {
    local iface="$1"
    systemctl stop "udhcpc@${iface}.service" >/dev/null 2>&1 || true
    local udhcpc_pids=""
    local pid cmdline
    for pid in $(pgrep -x udhcpc 2>/dev/null || true); do
        cmdline=$(tr '\0' ' ' <"/proc/${pid}/cmdline" 2>/dev/null || true)
        if [[ " $cmdline " == *" -i ${iface} "* ]] || [[ " $cmdline " == *" -i${iface} "* ]]; then
            udhcpc_pids+="${pid}"$'\n'
        fi
    done

    if [ -n "$udhcpc_pids" ]; then
        while IFS= read -r pid; do
            if [ -n "$pid" ]; then
                kill -TERM "$pid" 2>/dev/null && debug_print "[wifi] direct udhcpc (PID $pid) terminated"
            fi
        done <<<"$udhcpc_pids"
        sleep 0.2
        while IFS= read -r pid; do
            if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
                kill -KILL "$pid" 2>/dev/null || true
                debug_print "[wifi] direct udhcpc (PID $pid) force-killed"
            fi
        done <<<"$udhcpc_pids"
    fi
}

try_static_ip() {
    local iface="$1"
    local nodhcp_file="/boot/wifi.nodhcp"

    [[ -f "$nodhcp_file" ]] || return 1

    debug_print "[wifi] found /boot/wifi.nodhcp, attempting static IP assignment"

    while read -r line || [[ -n "$line" ]]; do
        [[ -z "$line" || "$line" == \#* ]] && continue

        if ! [[ "$line" =~ ^([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})/([0-9]{1,2})$ ]]; then
            debug_print "[wifi] skipping invalid entry in nodhcp file: $line"
            continue
        fi

        local ip="${BASH_REMATCH[1]}"
        debug_print "[wifi] trying static IP: $line"

        ip addr flush dev "$iface" 2>/dev/null || true

        local ip_free=true
        if command -v arping >/dev/null 2>&1; then
            if arping -D -S 0.0.0.0 -c 2 -w 2 -I "$iface" "$ip" >/dev/null 2>&1; then
                debug_print "[wifi] $ip is already in use on the network, trying next"
                ip_free=false
            fi
        fi

        if [[ "$ip_free" == "true" ]]; then
            ip addr add "$line" dev "$iface" 2>/dev/null || {
                debug_print "[wifi] failed to assign $line to $iface, skipping"
                continue
            }
            local gateway
            gateway=$(echo "$ip" | awk -F. '{print $1"."$2"."$3".1"}')
            ip route add default via "$gateway" dev "$iface" 2>/dev/null || true
            debug_print "[wifi] static IP configured: $line, gateway: $gateway"
            return 0
        fi
    done < "$nodhcp_file"

    debug_print "[wifi] no available static IP found in $nodhcp_file, falling back to DHCP"
    return 1
}

# Configure IP for STA mode: try static assignment first, fall back to DHCP.
configure_sta_ip() {
    local iface="$1"
    if try_static_ip "$iface"; then
        return 0
    fi
    start_dhcp_client "$iface"
}

start_service() {
    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface) || exit 1

    ensure_wpa_conf

    if ! pgrep -f "wpa_supplicant.*${WIFI_IFACE}" >/dev/null; then
        debug_print "[wifi] starting wpa_supplicant..."
        mkdir -p /var/run/wpa_supplicant
        wpa_supplicant -D "${DRIVER}" -i "$WIFI_IFACE" -c "$WPA_CONF_FILE" -B -P "$WPA_PID_FILE" >/dev/null 2>&1
        local WPA_PID=$(cat "$WPA_PID_FILE" 2>/dev/null)
        if [[ -n "$WPA_PID" ]]; then
            echo $WPA_PID >/sys/fs/cgroup/cgroup.procs 2>/dev/null || true
        fi
        sleep 1
    fi

    if [[ ! -S "/var/run/wpa_supplicant/$WIFI_IFACE" ]]; then
        debug_print "[wifi] warning: control socket not found!"
    else
        debug_print "[wifi] wpa_supplicant is running on $WIFI_IFACE"
    fi

    ip link set dev "$WIFI_IFACE" up
}

stop_service() {
    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface) || {
        debug_print "[wifi] no wireless interface found"
        return 1
    }

    debug_print "[wifi] stopping wpa_supplicant on $WIFI_IFACE..."

    stop_dhcp_client "$WIFI_IFACE"

    if [[ -f "$WPA_PID_FILE" ]]; then
        local pid
        pid=$(<"$WPA_PID_FILE")
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" && debug_print "[wifi] wpa_supplicant (PID $pid) terminated"
        fi
        rm -f "$WPA_PID_FILE"
    else
        pkill -f "wpa_supplicant.*${WIFI_IFACE}" 2>/dev/null || true
        pkill -f "wpa_supplicant" 2>/dev/null || true
    fi

    ip addr flush dev "$WIFI_IFACE"
}

start_wifi_system() {
    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface) || {
        echo "ERROR: NO_INTERFACE"
        return 1
    }

    # Stop dbus-mode wpa_supplicant if present to avoid conflicts
    if systemctl is-active wpa_supplicant >/dev/null 2>&1; then
        debug_print "[wifi] stopping dbus wpa_supplicant service"
        systemctl stop wpa_supplicant >/dev/null 2>&1 || true
        sleep 1
    fi

    # Clean up any stale per-interface wpa_supplicant instances
    pkill -f "wpa_supplicant.*${WIFI_IFACE}" 2>/dev/null || true
    sleep 0.5

    if [[ ! -f "$WIFI_CFG_PATH" ]]; then
        ensure_wpa_conf
        mark_saved_networks_disabled || true
    fi

    start_service

    local retry=0
    while [ $retry -lt 20 ]; do
        if [[ -S "/var/run/wpa_supplicant/${WIFI_IFACE}" ]]; then
            break
        fi
        sleep 0.5
        ((retry++))
    done

    if [[ ! -S "/var/run/wpa_supplicant/${WIFI_IFACE}" ]]; then
        echo "ERROR: WPA_NOT_RUNNING"
        return 1
    fi

    if [[ -f "$WIFI_CFG_PATH" ]]; then
        debug_print "[wifi] wifi.conf exists, connecting configured network..."
        try_connect || true
    else
        debug_print "[wifi] wifi.conf not found, disabling all networks (runtime fallback)"
        wpa_cli -i "$WIFI_IFACE" disable_network all >/dev/null 2>&1 || true
    fi

    return 0
}

stop_wifi_system() {
    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface 2>/dev/null || true)

    local status
    if [[ -n "$WIFI_IFACE" ]]; then
        status=$(get_connection_status 2>/dev/null || echo "DISCONNECTED")
    else
        status="ERROR: NO_INTERFACE"
    fi

    local boot_config_action="clear"
    local boot_config_ssid=""
    local need_disable_saved_networks="false"

    if [[ -z "$WIFI_IFACE" ]]; then
        boot_config_action="preserve-existing"
        debug_print "[wifi] stop snapshot: no wireless interface, preserving boot config"
    elif [[ "$status" == CONNECTED* ]]; then
        boot_config_ssid=$(echo "$status" | cut -d' ' -f2-)
        if [[ -n "$boot_config_ssid" ]]; then
            boot_config_action="keep-connected"
            debug_print "[wifi] stop snapshot: connected, planning to preserve boot config for ${boot_config_ssid}"
        fi
    elif [[ "$status" == AP* ]] || is_ap_runtime_active; then
        local ap_boot_decision
        ap_boot_decision=$(resolve_boot_config_action_from_ap_previous_state)
        if [[ "$ap_boot_decision" == keep-connected\|* ]]; then
            boot_config_action="keep-connected"
            boot_config_ssid=${ap_boot_decision#keep-connected|}
            debug_print "[wifi] stop snapshot: AP mode, planning to restore boot config for ${boot_config_ssid}"
        else
            debug_print "[wifi] stop snapshot: AP mode, planning to clear boot config"
        fi
    else
        need_disable_saved_networks="true"
        debug_print "[wifi] stop snapshot: disconnected, planning to clear boot config and disable saved networks"
    fi

    stop_wifi_ap
    stop_service

    if [ -n "$WIFI_IFACE" ]; then
        ip link set dev "$WIFI_IFACE" down 2>/dev/null || true
    fi

    if [[ "$boot_config_action" == "keep-connected" && -n "$boot_config_ssid" ]]; then
        write_wifi_boot_config "$boot_config_ssid"
    elif [[ "$boot_config_action" == "preserve-existing" ]]; then
        :
    else
        clear_wifi_boot_config
        if [[ "$need_disable_saved_networks" == "true" ]]; then
            ensure_wpa_conf
            mark_saved_networks_disabled || true
        fi
    fi
    clear_ap_previous_state

    echo "OK: WiFi stopped"
    return 0
}

start_wifi_ap() {
    local WIFI_IFACE=$(get_wireless_interface)
    local WIFI_SSID="$1"
    local WIFI_PSW="$2"

    if pgrep hostapd >/dev/null; then
        local current_ssid
        current_ssid=$(awk -F= '/^ssid=/{print $2}' "$HOSTAPD_CONF" 2>/dev/null)
        if [[ "$current_ssid" == "$WIFI_SSID" ]]; then
            debug_print "[wifi] AP '$WIFI_SSID' already active on $WIFI_IFACE, skipping reconfiguration"
            return 0
        else
            debug_print "[wifi] hostapd running with SSID '$current_ssid', restarting for new SSID '$WIFI_SSID'"
        fi
    fi

    save_ap_previous_state
    disconnect_wifi "ap-switch"
    stop_service

    local retry=0
    while pgrep -f "wpa_supplicant.*${WIFI_IFACE}" >/dev/null && [ $retry -lt 10 ]; do
        sleep 0.5
        ((retry++))
    done

    if pgrep -f "wpa_supplicant.*${WIFI_IFACE}" >/dev/null; then
        pkill -9 -f "wpa_supplicant.*${WIFI_IFACE}" || true
        sleep 0.5
    fi

    pkill hostapd || true
    pkill udhcpd || true

    debug_print "[wifi] configuring interface: ${WIFI_IFACE}"
    ip addr flush dev "$WIFI_IFACE"
    ip addr add "$GATEWAY/24" dev "$WIFI_IFACE"
    ip link set dev "$WIFI_IFACE" up

    debug_print "[wifi] generating hostapd configuration"
    if [[ -z "$WIFI_PSW" ]]; then
        cat >"$HOSTAPD_CONF" <<EOF
interface=$WIFI_IFACE
driver=nl80211
ssid=$WIFI_SSID
hw_mode=g
channel=6
ieee80211n=1
wmm_enabled=1
ht_capab=[HT40+][SHORT-GI-20][SHORT-GI-40]
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
ctrl_interface=/var/run/hostapd
ctrl_interface_group=0
EOF
    else
        cat >"$HOSTAPD_CONF" <<EOF
interface=$WIFI_IFACE
driver=nl80211
ssid=$WIFI_SSID
hw_mode=g
channel=6
ieee80211n=1
wmm_enabled=1
ht_capab=[HT40+][SHORT-GI-20][SHORT-GI-40]
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase=$WIFI_PSW
wpa_key_mgmt=WPA-PSK
rsn_pairwise=CCMP
ctrl_interface=/var/run/hostapd
ctrl_interface_group=0
EOF
    fi

    debug_print "[wifi] starting hostapd service"
    if ! hostapd -B "$HOSTAPD_CONF" >/dev/null 2>&1; then
        debug_print "[wifi] failed to start hostapd, rolling back AP previous state"
        clear_ap_previous_state
        return 1
    fi

    local ap_retry=0
    while [ $ap_retry -lt 6 ]; do
        if pgrep hostapd >/dev/null 2>&1; then
            debug_print "[wifi] hostapd is running"
            break
        fi
        sleep 0.5
        ((ap_retry++))
    done
    if ! pgrep hostapd >/dev/null 2>&1; then
        debug_print "[wifi] hostapd did not stay running, rolling back AP previous state"
        clear_ap_previous_state
        return 1
    fi

    debug_print "[wifi] generating udhcpd configuration"
    cat >"$UDHCPD_CONF" <<EOF
start           ${SUBNET}.100
end             ${SUBNET}.200
interface       ${WIFI_IFACE}
opt     subnet  255.255.255.0
opt     router  ${GATEWAY}
opt     dns     8.8.8.8
lease_file      ${TMP_DIR}/udhcpd.leases
option  lease   86400
EOF

    debug_print "[wifi] starting udhcpd service"
    if ! udhcpd "$UDHCPD_CONF" >/dev/null 2>&1; then
        debug_print "[wifi] failed to start udhcpd, rolling back AP previous state"
        clear_ap_previous_state
        return 1
    fi

    debug_print "[wifi] configuring network routing"
    sysctl -w net.ipv4.ip_forward=1 >/dev/null || true
    iptables -t nat -A POSTROUTING -o "$ETH_IFACE" -j MASQUERADE 2>/dev/null || true
    iptables -A FORWARD -i "$WIFI_IFACE" -o "$ETH_IFACE" -j ACCEPT 2>/dev/null || true
    iptables -A FORWARD -i "$ETH_IFACE" -o "$WIFI_IFACE" -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null || true

    debug_print "[wifi] access point activated - SSID: ${WIFI_SSID} | Password: ${WIFI_PSW}"
    return 0
}

stop_wifi_ap() {
    local WIFI_IFACE=$(get_wireless_interface)

    debug_print "[wifi] terminating ap services"
    pkill hostapd || true
    pkill udhcpd || true
    sleep 0.5

    rm -f "$HOSTAPD_CONF" || true

    debug_print "[wifi] cleaning network configuration"
    iptables -t nat -D POSTROUTING -o "$ETH_IFACE" -j MASQUERADE 2>/dev/null || true
    if [[ -n "$WIFI_IFACE" ]]; then
        iptables -D FORWARD -i "$WIFI_IFACE" -o "$ETH_IFACE" -j ACCEPT 2>/dev/null || true
        iptables -D FORWARD -i "$ETH_IFACE" -o "$WIFI_IFACE" -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null || true
    fi

    sysctl -w net.ipv4.ip_forward=0 >/dev/null

    if [[ -n "$WIFI_IFACE" ]]; then
        debug_print "[wifi] resetting interface configuration: ${WIFI_IFACE}"
        ip addr flush dev "$WIFI_IFACE"
    else
        debug_print "[wifi] no wireless interface available while stopping AP"
    fi
    clear_ap_previous_state

    debug_print "[wifi] access point deactivated successfully"
}

device_check() {
    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface) || {
        echo "0"
        return 1
    }

    local count
    count=$(hostapd_cli -i "$WIFI_IFACE" list_sta 2>/dev/null | grep -cE '^[0-9a-f]{2}(:[0-9a-f]{2}){5}$' 2>/dev/null) || count=0
    echo "$count"

    return 0
}

get_ap_ip() {
    echo "$GATEWAY"
}

try_scan() {
    local WIFI_IFACE=$(get_wireless_interface)

    if [ -z "$WIFI_IFACE" ]; then
        debug_print "[wifi] no wireless interface found" >&2
        echo "[]"
        return 1
    fi

    start_service
    debug_print "[wifi] scanning for available networks..."

    if ! wpa_cli -i "${WIFI_IFACE}" scan >/dev/null 2>&1; then
        debug_print "[wifi] scan command failed" >&2
        echo "[]"
        return 1
    fi

    sleep 2

    local scan_output
    scan_output=$(wpa_cli -i "${WIFI_IFACE}" scan_results 2>/dev/null)

    if [ -z "$scan_output" ]; then
        debug_print "[wifi] no scan results received" >&2
        echo "[]"
        return 1
    fi

    echo "$scan_output" | awk '
    NR > 1 && NF >= 5 {
        # Skip header line
        bssid = $1
        frequency = $2
        signal = $3
        flags = $4

        # Extract SSID (everything after 4th field)
        ssid = ""
        for (i = 5; i <= NF; i++) {
            ssid = ssid (i > 5 ? " " : "") $i
        }

        # Determine security type
        security = "OPEN"
        if (index(flags, "WPA2-EAP") > 0 || index(flags, "WPA-EAP") > 0) security = "EAP"
        else if (index(flags, "EAP") > 0) security = "EAP"
        else if (index(flags, "SAE") > 0) security = "WPA3"
        else if (index(flags, "WPA2-PSK") > 0 || index(flags, "WPA2") > 0) security = "WPA2"
        else if (index(flags, "WPA-PSK") > 0 || index(flags, "WPA") > 0) security = "WPA"
        else if (index(flags, "WEP") > 0) security = "WEP"

        # Skip empty SSIDs
        if (length(ssid) == 0) next

        # Store network info using composite key, keep only the strongest signal for each SSID
        key = ssid
        if (!(key in seen) || signal > ssid_signal[key]) {
            ssid_ssid[key] = ssid
            ssid_bssid[key] = bssid
            ssid_signal[key] = signal
            ssid_freq[key] = frequency
            ssid_sec[key] = security
            seen[key] = 1
        }
    }
    END {
        # Sort by signal strength (descending)
        n = 0
        for (key in seen) {
            sorted[n] = key
            n++
        }

        # Bubble sort by signal strength
        for (i = 0; i < n-1; i++) {
            for (j = 0; j < n-i-1; j++) {
                if (ssid_signal[sorted[j]] < ssid_signal[sorted[j+1]]) {
                    temp = sorted[j]
                    sorted[j] = sorted[j+1]
                    sorted[j+1] = temp
                }
            }
        }

        # Output JSON
        print "["
        for (i = 0; i < n; i++) {
            key = sorted[i]
            if (i > 0) print ","
            printf "  {\"ssid\":\"%s\",\"bssid\":\"%s\",\"signal\":%d,\"frequency\":%d,\"security\":\"%s\"}",
                   ssid_ssid[key],
                   ssid_bssid[key],
                   ssid_signal[key],
                   ssid_freq[key],
                   ssid_sec[key]
        }
        print ""
        print "]"
    }'

    return 0
}

try_connect() {
    local WIFI_IFACE=$(get_wireless_interface)

    if [ -z "$WIFI_IFACE" ]; then
        echo "false" >"$HW_WIFI_CFG_PATH"
        return 1
    else
        echo "true" >"$HW_WIFI_CFG_PATH"
    fi

    if [[ ! -r "$WIFI_CFG_PATH" ]]; then
        debug_print "[wifi] config file not found or inaccessible: $WIFI_CFG_PATH, skip" >&2
        return 2
    fi

    WIFI_CFG_SSID=""
    while IFS="=" read -r key value; do
        case "$key" in
        "WIFI_CFG_SSID") WIFI_CFG_SSID="$value" ;;
        esac
    done <"$WIFI_CFG_PATH"

    if [[ -z "$WIFI_CFG_SSID" ]]; then
        debug_print "[wifi] missing SSID in config" >&2
        return 3
    fi

    debug_print "[wifi] connecting to SSID: $WIFI_CFG_SSID"

    if ! connect_wifi "$WIFI_CFG_SSID"; then
        debug_print "[wifi] error: failed to connect to SSID: $WIFI_CFG_SSID" >&2
        return 4
    fi

    return 0
}

escape_non_ascii() {
    local input="$1"
    local escaped=""
    local i c ord b

    for ((i = 0; i < ${#input}; i++)); do
        c="${input:i:1}"
        LC_CTYPE=C
        ord=$(printf "%d" "'$c" 2>/dev/null || ord=0)
        if [ "$ord" -ge 32 ] && [ "$ord" -le 126 ]; then
            if [ "$c" = "\\" ]; then
                escaped+="\\\\"
            else
                escaped+="$c"
            fi
        else
            while read -r b; do
                escaped+="\\x$b"
            done < <(echo -n "$c" | xxd -p -c1)
        fi
    done

    printf '%s' "$escaped"
}

unescape_non_ascii() {
    local input="$1"
    local result=""
    local i=0

    while [ $i -lt ${#input} ]; do
        local c="${input:i:1}"

        if [ "$c" = "\\" ]; then
            local next="${input:i+1:1}"

            if [ "$next" = "\\" ]; then
                result+="\\\\"
                ((i += 2))
            elif [ "$next" = "x" ]; then
                local hex="${input:i+2:2}"
                if [[ "$hex" =~ ^[0-9a-fA-F]{2}$ ]]; then
                    result+=$(printf "\\x$hex")
                    ((i += 4))
                else
                    result+="$c"
                    ((i += 1))
                fi
            else
                result+="$c"
                ((i += 1))
            fi
        else
            result+="$c"
            ((i += 1))
        fi
    done

    printf '%b' "$result"
}

decode_non_ascii_x_escape_run() {
    local hex_run="$1"
    local offset hex value
    local has_non_ascii="false"

    for ((offset = 0; offset < ${#hex_run}; offset += 2)); do
        hex="${hex_run:offset:2}"
        value=$((16#$hex))
        if [ "$value" -ge 128 ]; then
            has_non_ascii="true"
            break
        fi
    done

    if [[ "$has_non_ascii" != "true" ]]; then
        return 1
    fi

    printf '%s' "$hex_run" | xxd -r -p
}

normalize_ssid_arg() {
    local input="$1"
    local output=""
    local i=0
    local c next hex start hex_run decoded

    while [ $i -lt ${#input} ]; do
        c="${input:i:1}"
        next="${input:i+1:1}"
        hex="${input:i+2:2}"

        if [[ "$c" == "\\" && "$next" == "x" && "$hex" =~ ^[0-9a-fA-F]{2}$ ]]; then
            start=$i
            hex_run=""
            while [ $((i + 3)) -lt ${#input} ]; do
                hex="${input:i+2:2}"
                if [[ "${input:i:2}" != "\\x" || ! "$hex" =~ ^[0-9a-fA-F]{2}$ ]]; then
                    break
                fi
                hex_run+="$hex"
                ((i += 4))
            done

            if decoded=$(decode_non_ascii_x_escape_run "$hex_run"); then
                output+="$decoded"
            else
                output+="${input:start:i-start}"
            fi
        else
            output+="$c"
            ((i += 1))
        fi
    done

    printf '%s' "$output"
}

connect_wifi() {
    local ssid
    ssid=$(normalize_ssid_arg "$1")
    local password="$2"
    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface)

    start_service

    if [[ ! -S "/var/run/wpa_supplicant/${WIFI_IFACE}" ]]; then
        echo "[wifi] wpa_supplicant socket not found for $WIFI_IFACE" >&2
        return 1
    fi

    local ssid_escaped
    ssid_escaped=$(escape_non_ascii "$ssid")
    debug_print "[wifi] ssid_escaped: $ssid_escaped"

    local existing
    existing=$(wpa_cli -i "$WIFI_IFACE" list_networks | grep -F "$ssid_escaped" | awk '{print $1}' | head -n1)
    debug_print "[wifi] existing network id: $existing"

    if [[ -n "$password" && -n "$existing" ]]; then
        debug_print "[wifi] password provided, deleting existing network id=$existing"
        wpa_cli -i "$WIFI_IFACE" remove_network "$existing" >/dev/null
        existing=""
    fi

    wpa_cli -i "$WIFI_IFACE" disable_network all >/dev/null

    if [[ -n "$existing" ]]; then
        debug_print "[wifi] found existing network id=$existing, reusing."
        wpa_cli -i "$WIFI_IFACE" set_network "$existing" mesh_fwding 0 >/dev/null
        wpa_cli -i "$WIFI_IFACE" enable_network "$existing" >/dev/null
        wpa_cli -i "$WIFI_IFACE" select_network "$existing" >/dev/null
        wpa_cli -i "$WIFI_IFACE" save_config >/dev/null
    else
        debug_print "[wifi] adding new network: $ssid"
        local id
        id=$(wpa_cli -i "$WIFI_IFACE" add_network | tail -n1)
        wpa_cli -i "$WIFI_IFACE" set_network "$id" ssid "\"$ssid\"" >/dev/null

        if [[ -z "$password" ]]; then
            wpa_cli -i "$WIFI_IFACE" set_network "$id" key_mgmt NONE >/dev/null
        else
            local psk
            psk=$(wpa_passphrase "$ssid" "$password" 2>/dev/null | awk -F= '/^[ \t]*psk=[^#]/{print $2; exit}')
            wpa_cli -i "$WIFI_IFACE" set_network "$id" psk "$psk" >/dev/null
        fi

        wpa_cli -i "$WIFI_IFACE" set_network "$id" mesh_fwding 0 >/dev/null
        wpa_cli -i "$WIFI_IFACE" enable_network "$id" >/dev/null
        wpa_cli -i "$WIFI_IFACE" select_network "$id" >/dev/null
        wpa_cli -i "$WIFI_IFACE" save_config >/dev/null
        debug_print "[wifi] saved new WiFi to config"
    fi

    debug_print "[wifi] waiting for connection..."
    local retry=0
    while [ $retry -lt 10 ]; do
        local status_output current_ssid
        status_output=$(wpa_cli -i "$WIFI_IFACE" status 2>/dev/null)
        current_ssid=$(echo "$status_output" | grep "^ssid=" | cut -d= -f2-)
        if echo "$status_output" | grep -qF "wpa_state=COMPLETED" &&
            { [[ "$current_ssid" == "$ssid" ]] || [[ "$current_ssid" == "$ssid_escaped" ]]; }; then
            debug_print "[wifi] connection established"
            break
        fi
        sleep 1
        ((retry++))
    done

    if [ $retry -eq 10 ]; then
        debug_print "[wifi] connection timeout" >&2
        return 1
    fi

    debug_print "[wifi] requesting IP address..."
    if ! configure_sta_ip "$WIFI_IFACE"; then
        echo "[wifi] IP configuration failed on $WIFI_IFACE" >&2
        return 1
    fi

    cat <<EOF >"$WIFI_CFG_PATH"
WIFI_CFG_SSID=${ssid}
EOF

    debug_print "[wifi] connected to $ssid"
    return 0
}

connect_enterprise_wifi() {
    local ssid
    ssid=$(normalize_ssid_arg "$1")
    local eap_method="$2"
    local identity="$3"
    shift 3

    local password="" phase2="" ca_cert="" client_cert="" private_key="" private_key_passwd=""
    local anonymous_identity="" domain=""

    for param in "$@"; do
        case "$param" in
        password=*) password="${param#password=}" ;;
        phase2=*) phase2="${param#phase2=}" ;;
        ca_cert=*) ca_cert="${param#ca_cert=}" ;;
        client_cert=*) client_cert="${param#client_cert=}" ;;
        private_key=*) private_key="${param#private_key=}" ;;
        private_key_passwd=*) private_key_passwd="${param#private_key_passwd=}" ;;
        anonymous_identity=*) anonymous_identity="${param#anonymous_identity=}" ;;
        domain=*) domain="${param#domain=}" ;;
        -d | --debug) ;;
        *)
            debug_print "[wifi] unknown enterprise parameter: $param" >&2
            ;;
        esac
    done

    eap_method=$(echo "$eap_method" | tr '[:lower:]' '[:upper:]')

    case "$eap_method" in
    PEAP | TLS | TTLS) ;;
    *)
        echo "[wifi] unsupported EAP method: $eap_method (supported: PEAP, TLS, TTLS)" >&2
        return 1
        ;;
    esac

    case "$eap_method" in
    PEAP)
        if [[ -z "$password" ]]; then
            echo "[wifi] PEAP requires password parameter" >&2
            return 1
        fi
        [[ -z "$phase2" ]] && phase2="auth=MSCHAPV2"
        ;;
    TLS)
        if [[ -z "$client_cert" || -z "$private_key" ]]; then
            echo "[wifi] EAP-TLS requires client_cert and private_key parameters" >&2
            return 1
        fi
        if [[ ! -f "$client_cert" ]]; then
            echo "[wifi] client certificate not found: $client_cert" >&2
            return 1
        fi
        if [[ ! -f "$private_key" ]]; then
            echo "[wifi] private key not found: $private_key" >&2
            return 1
        fi
        ;;
    TTLS)
        if [[ -z "$password" ]]; then
            echo "[wifi] EAP-TTLS requires password parameter" >&2
            return 1
        fi
        [[ -z "$phase2" ]] && phase2="auth=PAP"
        ;;
    esac

    if [[ -n "$ca_cert" && ! -f "$ca_cert" ]]; then
        echo "[wifi] CA certificate not found: $ca_cert" >&2
        return 1
    fi

    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface)

    start_service

    if [[ ! -S "/var/run/wpa_supplicant/${WIFI_IFACE}" ]]; then
        echo "[wifi] wpa_supplicant socket not found for $WIFI_IFACE" >&2
        return 1
    fi

    local ssid_escaped
    ssid_escaped=$(escape_non_ascii "$ssid")
    debug_print "[wifi] enterprise connect: ssid=$ssid eap=$eap_method identity=$identity"

    local existing
    existing=$(wpa_cli -i "$WIFI_IFACE" list_networks | grep -F "$ssid_escaped" | awk '{print $1}' | head -n1)
    if [[ -n "$existing" ]]; then
        debug_print "[wifi] removing existing network id=$existing for reconfiguration"
        wpa_cli -i "$WIFI_IFACE" remove_network "$existing" >/dev/null
    fi

    local id
    id=$(wpa_cli -i "$WIFI_IFACE" add_network | tail -n1)
    debug_print "[wifi] added enterprise network id=$id"

    wpa_cli -i "$WIFI_IFACE" set_network "$id" ssid "\"$ssid\"" >/dev/null
    wpa_cli -i "$WIFI_IFACE" set_network "$id" key_mgmt "WPA-EAP" >/dev/null
    wpa_cli -i "$WIFI_IFACE" set_network "$id" eap "$eap_method" >/dev/null
    wpa_cli -i "$WIFI_IFACE" set_network "$id" identity "\"$identity\"" >/dev/null

    if [[ -n "$anonymous_identity" ]]; then
        wpa_cli -i "$WIFI_IFACE" set_network "$id" anonymous_identity "\"$anonymous_identity\"" >/dev/null
    fi

    if [[ -n "$ca_cert" ]]; then
        wpa_cli -i "$WIFI_IFACE" set_network "$id" ca_cert "\"$ca_cert\"" >/dev/null
    fi

    if [[ -n "$domain" ]]; then
        wpa_cli -i "$WIFI_IFACE" set_network "$id" domain_suffix_match "\"$domain\"" >/dev/null
    fi

    case "$eap_method" in
    PEAP)
        wpa_cli -i "$WIFI_IFACE" set_network "$id" password "\"$password\"" >/dev/null
        wpa_cli -i "$WIFI_IFACE" set_network "$id" phase2 "\"$phase2\"" >/dev/null
        debug_print "[wifi] PEAP configured with phase2=$phase2"
        ;;
    TLS)
        wpa_cli -i "$WIFI_IFACE" set_network "$id" client_cert "\"$client_cert\"" >/dev/null
        wpa_cli -i "$WIFI_IFACE" set_network "$id" private_key "\"$private_key\"" >/dev/null
        if [[ -n "$private_key_passwd" ]]; then
            wpa_cli -i "$WIFI_IFACE" set_network "$id" private_key_passwd "\"$private_key_passwd\"" >/dev/null
        fi
        debug_print "[wifi] EAP-TLS configured with cert=$client_cert key=$private_key"
        ;;
    TTLS)
        wpa_cli -i "$WIFI_IFACE" set_network "$id" password "\"$password\"" >/dev/null
        wpa_cli -i "$WIFI_IFACE" set_network "$id" phase2 "\"$phase2\"" >/dev/null
        debug_print "[wifi] EAP-TTLS configured with phase2=$phase2"
        ;;
    esac

    wpa_cli -i "$WIFI_IFACE" disable_network all >/dev/null
    wpa_cli -i "$WIFI_IFACE" set_network "$id" mesh_fwding 0 >/dev/null
    wpa_cli -i "$WIFI_IFACE" enable_network "$id" >/dev/null
    wpa_cli -i "$WIFI_IFACE" select_network "$id" >/dev/null
    wpa_cli -i "$WIFI_IFACE" save_config >/dev/null

    debug_print "[wifi] waiting for enterprise connection..."
    local retry=0
    while [ $retry -lt 20 ]; do
        local status_output state current_ssid
        status_output=$(wpa_cli -i "$WIFI_IFACE" status 2>/dev/null)
        state=$(echo "$status_output" | grep "wpa_state=" | cut -d= -f2)
        current_ssid=$(echo "$status_output" | grep "^ssid=" | cut -d= -f2-)
        if [[ "$state" == "COMPLETED" ]] &&
            { [[ "$current_ssid" == "$ssid" ]] || [[ "$current_ssid" == "$ssid_escaped" ]]; }; then
            debug_print "[wifi] enterprise connection established"
            break
        fi
        sleep 1
        ((retry++))
    done

    if [ $retry -eq 20 ]; then
        echo "[wifi] enterprise connection timeout after 20 seconds" >&2
        local status_output
        status_output=$(wpa_cli -i "$WIFI_IFACE" status 2>/dev/null)
        local eap_status
        eap_status=$(echo "$status_output" | grep "EAP state=" | cut -d= -f2)
        if [[ -n "$eap_status" ]]; then
            debug_print "[wifi] EAP state at timeout: $eap_status"
        fi
        return 1
    fi

    debug_print "[wifi] requesting IP address..."
    if ! configure_sta_ip "$WIFI_IFACE"; then
        echo "[wifi] IP configuration failed on $WIFI_IFACE" >&2
        return 1
    fi

    cat <<EOF >"$WIFI_CFG_PATH"
WIFI_CFG_SSID=${ssid}
EOF

    debug_print "[wifi] connected to enterprise network: $ssid (EAP: $eap_method)"
    return 0
}

disconnect_wifi() {
    local disconnect_reason="${1:-manual-off}"
    local preserve_boot_marker="false"
    if [[ "$disconnect_reason" == "ap-switch" ]]; then
        preserve_boot_marker="true"
    fi

    debug_print "[wifi] disconnecting..."
    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface 2>/dev/null || true)

    if [[ -z "$WIFI_IFACE" ]]; then
        debug_print "[wifi] no wireless interface, preserving wifi config file"
        return 0
    fi

    stop_dhcp_client "$WIFI_IFACE"

    if [[ -S "/var/run/wpa_supplicant/$WIFI_IFACE" ]]; then
        start_service

        local networks=$(list_networks)
        local network_ssid=$(echo "$networks" | grep -o '{"ssid":"[^"]*","flags":"CURRENT"}' | sed -n 's/.*"ssid":"\([^"]*\)".*/\1/p')
        local network_id=$(wpa_cli -i "$WIFI_IFACE" list_networks | grep '\[CURRENT\]' | awk '{print $1}')

        if [[ -n "$network_id" ]]; then
            debug_print "[wifi] disabling network id=$network_id, ssid=$network_ssid"
            wpa_cli -i "$WIFI_IFACE" disable_network "$network_id" >/dev/null
            echo "$(unescape_non_ascii "$network_ssid")" >"$PREVIOUS_WIFI_SAVE"
            echo "$(unescape_non_ascii "$network_ssid")" >"$LAST_WIFI_FILE"
        fi

        wpa_cli -i "$WIFI_IFACE" disconnect >/dev/null
        wpa_cli -i "$WIFI_IFACE" save_config >/dev/null
    fi

    ip addr flush dev "${WIFI_IFACE}"

    if [[ -f "${WIFI_CFG_PATH}" && "$preserve_boot_marker" != "true" ]]; then
        rm -f "${WIFI_CFG_PATH}"
        debug_print "[wifi] removed wifi config file: ${WIFI_CFG_PATH}"
    fi

    # Persist disabled state into wpa_supplicant.conf so that
    # wpa_supplicant will NOT auto-reconnect on next start.
    mark_saved_networks_disabled || true

    return 0
}

remove_wifi() {
    local ssid="$1"
    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface)

    start_service

    if [[ ! -S "/var/run/wpa_supplicant/${WIFI_IFACE}" ]]; then
        debug_print "[wifi] wpa_supplicant socket not found for $WIFI_IFACE" >&2
        return 1
    fi

    local ssid_escaped
    ssid_escaped=$(escape_non_ascii "$ssid")
    debug_print "[wifi] removing network: $ssid (escaped: $ssid_escaped)"

    local network_id
    network_id=$(wpa_cli -i "$WIFI_IFACE" list_networks | grep -F "$ssid_escaped" | awk '{print $1}' | head -n1)

    if [[ -z "$network_id" ]]; then
        debug_print "[wifi] network not found: $ssid" >&2
        return 1
    fi

    local is_current
    is_current=$(wpa_cli -i "$WIFI_IFACE" list_networks | grep -F "$ssid_escaped" | grep -q '\[CURRENT\]' && echo "true" || echo "false")

    if [[ "$is_current" == "true" ]]; then
        debug_print "[wifi] network is currently connected, disconnecting first..."
        disconnect_wifi
        sleep 1
    fi

    debug_print "[wifi] found network id=$network_id, removing..."

    if wpa_cli -i "$WIFI_IFACE" remove_network "$network_id" >/dev/null; then
        wpa_cli -i "$WIFI_IFACE" save_config >/dev/null
        debug_print "[wifi] network removed and config saved: $ssid"

        if [[ -f "$WIFI_CFG_PATH" ]]; then
            local cfg_ssid
            cfg_ssid=$(grep -E '^WIFI_CFG_SSID=' "$WIFI_CFG_PATH" | cut -d= -f2-)
            if [[ "$cfg_ssid" == "$ssid" ]]; then
                rm -f "$WIFI_CFG_PATH"
                debug_print "[wifi] removed wifi config file as it matched removed network"
            fi
        fi
    else
        debug_print "[wifi] failed to remove network: $ssid" >&2
        return 1
    fi

    return 0
}

wireless_ip_exists() {
    local WIFI_IFACE=$(get_wireless_interface)

    if [ -z "$WIFI_IFACE" ]; then
        echo "false"
        return 1
    fi

    if ! ip link show "$WIFI_IFACE" &>/dev/null; then
        echo "false"
        return 1
    fi

    if ip addr show "$WIFI_IFACE" 2>/dev/null | grep -E 'inet ' &>/dev/null; then
        echo "true"
        return 0
    else
        echo "false"
        return 0
    fi
}

has_wifi_config() {
    if [[ ! -f "$WIFI_CFG_PATH" ]]; then
        echo "false"
        return 1
    fi

    local ssid
    ssid=$(grep -E '^WIFI_CFG_SSID=' "$WIFI_CFG_PATH" | cut -d= -f2-)

    if [[ -n "$ssid" ]]; then
        echo "true"
    else
        echo "false"
    fi

    return 0
}

list_networks() {
    local WIFI_IFACE=$(get_wireless_interface)

    if [ -z "$WIFI_IFACE" ]; then
        echo "[]"
        return 1
    fi

    if [[ ! -S "/var/run/wpa_supplicant/$WIFI_IFACE" ]]; then
        echo "[]"
        return 1
    fi

    local networks_output
    networks_output=$(wpa_cli -i "$WIFI_IFACE" list_networks 2>/dev/null)

    if [ -z "$networks_output" ]; then
        echo "[]"
        return 1
    fi

    echo "$networks_output" | awk '
    NR > 1 && NF >= 3 {
        network_id = $1

        if ($NF ~ /^\[/) {
            flags = $NF
            bssid = $(NF-1)
            ssid = ""
            for (i = 2; i < NF - 1; i++) {
                ssid = ssid (i > 2 ? " " : "") $i
            }
        } else {
            flags = ""
            bssid = $NF
            ssid = ""
            for (i = 2; i < NF; i++) {
                ssid = ssid (i > 2 ? " " : "") $i
            }
        }

        flag_status = ""
        is_current = 0
        if (index(flags, "[CURRENT]") > 0) {
            flag_status = "CURRENT"
            is_current = 1
        } else if (index(flags, "[DISABLED]") > 0) {
            flag_status = "DISABLED"
        } else {
            flag_status = ""
        }

        network_json = sprintf("{\"ssid\":\"%s\",\"flags\":\"%s\"}", ssid, flag_status)

        if (is_current) {
            current_network = network_json
        } else {
            other_networks[other_count++] = network_json
        }
    }
    END {
        print "["

        count = 0
        if (current_network) {
            printf "  %s", current_network
            count++
        }

        for (i = 0; i < other_count; i++) {
            if (count > 0) print ","
            printf "  %s", other_networks[i]
            count++
        }

        print ""
        print "]"
    }'

    return 0
}

check_previous_wifi() {
    local WIFI_IFACE=$(get_wireless_interface)

    if [ -z "$WIFI_IFACE" ]; then
        echo "false"
        return 1
    fi

    start_service

    rm -f "$PREVIOUS_WIFI" || true

    if wpa_cli -i "$WIFI_IFACE" status 2>/dev/null | grep -qF "wpa_state=COMPLETED"; then
        debug_print "[wifi] already connected to WiFi"
        echo "false"
        return 0
    fi

    if [[ ! -f "$PREVIOUS_WIFI_SAVE" ]]; then
        debug_print "[wifi] no previous WiFi save file found"
        echo "false"
        return 0
    fi

    local saved_ssid=$(<"$PREVIOUS_WIFI_SAVE")
    local unescaped_ssid=$(escape_non_ascii "$saved_ssid")
    if [[ -z "$saved_ssid" ]]; then
        debug_print "[wifi] previous WiFi SSID is empty"
        echo "false"
        return 0
    fi

    local scan_results=$(try_scan)

    if echo "$scan_results" | grep -qF "\"ssid\":\"$unescaped_ssid\""; then
        debug_print "[wifi] saved network $saved_ssid is available in scan results"
        echo "true"
        cp "$PREVIOUS_WIFI_SAVE" "$PREVIOUS_WIFI"
    else
        debug_print "[wifi] saved network $saved_ssid is not available in scan results"
        echo "false"
        return 0
    fi

    return 0
}

# ========== New high-level helper functions ==========

get_connection_status() {
    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface) || {
        echo "ERROR: NO_INTERFACE"
        return 1
    }

    if pgrep hostapd >/dev/null 2>&1; then
        local ap_ssid
        ap_ssid=$(awk -F= '/^ssid=/{print $2}' "$HOSTAPD_CONF" 2>/dev/null)
        echo "AP ${ap_ssid:-unknown}"
        return 0
    fi

    if [[ -S "/var/run/wpa_supplicant/$WIFI_IFACE" ]]; then
        local state
        state=$(wpa_cli -i "$WIFI_IFACE" status 2>/dev/null | grep "^wpa_state=" | cut -d= -f2)
        if [[ "$state" == "COMPLETED" ]]; then
            local ssid
            ssid=$(wpa_cli -i "$WIFI_IFACE" status 2>/dev/null | grep "^ssid=" | cut -d= -f2)
            echo "CONNECTED ${ssid}"
            return 0
        fi
    fi

    echo "DISCONNECTED"
    return 0
}

get_signal_strength() {
    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface) || {
        echo "ERROR: NO_INTERFACE"
        return 1
    }

    if [[ ! -S "/var/run/wpa_supplicant/$WIFI_IFACE" ]]; then
        echo "ERROR: NOT_CONNECTED"
        return 1
    fi

    local state
    state=$(wpa_cli -i "$WIFI_IFACE" status 2>/dev/null | grep "^wpa_state=" | cut -d= -f2)
    if [[ "$state" != "COMPLETED" ]]; then
        echo "ERROR: NOT_CONNECTED"
        return 1
    fi

    local rssi
    rssi=$(wpa_cli -i "$WIFI_IFACE" signal_poll 2>/dev/null | grep "^RSSI=" | cut -d= -f2)
    if [[ -n "$rssi" ]]; then
        echo "$rssi"
        return 0
    fi

    rssi=$(iw dev "$WIFI_IFACE" station dump 2>/dev/null | grep "signal:" | awk '{print $2}')
    if [[ -n "$rssi" ]]; then
        echo "$rssi"
        return 0
    fi

    echo "ERROR: SIGNAL_NOT_AVAILABLE"
    return 1
}

get_saved_networks() {
    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface)

    if [ -z "$WIFI_IFACE" ]; then
        echo "[]"
        return 1
    fi

    if [[ ! -S "/var/run/wpa_supplicant/$WIFI_IFACE" ]]; then
        echo "[]"
        return 1
    fi

    local networks_output
    networks_output=$(wpa_cli -i "$WIFI_IFACE" list_networks 2>/dev/null)

    if [ -z "$networks_output" ]; then
        echo "[]"
        return 1
    fi

    local output="["
    local first=1
    while IFS=$'\t' read -r net_id ssid bssid flags rest; do
        [ -z "$ssid" ] && continue
        [ "$net_id" = "network" ] && continue
        if [ $first -eq 1 ]; then
            first=0
        else
            output+=","
        fi
        output+="{\"ssid\":\"$ssid\"}"
    done < <(echo "$networks_output" | tail -n +2)
    output+="]"
    echo "$output"
}

clear_saved_networks() {
    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface) || {
        echo "ERROR: NO_INTERFACE"
        return 1
    }

    start_service

    if [[ -S "/var/run/wpa_supplicant/$WIFI_IFACE" ]]; then
        wpa_cli -i "$WIFI_IFACE" disconnect >/dev/null 2>&1
        wpa_cli -i "$WIFI_IFACE" remove_network all >/dev/null 2>&1
        wpa_cli -i "$WIFI_IFACE" save_config >/dev/null 2>&1
    fi

    ip addr flush dev "$WIFI_IFACE" 2>/dev/null || true
    rm -f "$LAST_WIFI_FILE" || true
    rm -f "$PREVIOUS_WIFI_SAVE" || true
    rm -f "$PREVIOUS_WIFI" || true
    rm -f "$WIFI_CFG_PATH" || true

    echo "OK: All records cleared"
    return 0
}

turn_on_wifi() {
    local invocation_context="${1:-manual}"
    local persist_boot_marker="false"
    if [[ "$invocation_context" == "manual" ]]; then
        persist_boot_marker="true"
    fi

    local WIFI_IFACE
    WIFI_IFACE=$(get_wireless_interface) || {
        echo "ERROR: NO_INTERFACE"
        return 1
    }

    local status
    status=$(get_connection_status)
    if [[ "$status" == AP* ]]; then
        echo "ERROR: ALREADY_CONNECTED"
        return 1
    fi

    if [[ "$status" == CONNECTED* ]]; then
        debug_print "[wifi] already connected, ensuring IP is configured"
        if ! configure_sta_ip "$WIFI_IFACE"; then
            echo "ERROR: DHCP_FAILED"
            return 1
        fi
        local ssid
        ssid=$(echo "$status" | cut -d' ' -f2-)
        if [[ "$persist_boot_marker" == "true" ]]; then
            write_wifi_boot_config "$ssid"
        fi
        echo "OK: Connected to $ssid"
        return 0
    fi

    start_service

    if [[ ! -S "/var/run/wpa_supplicant/$WIFI_IFACE" ]]; then
        echo "ERROR: WPA_NOT_RUNNING"
        return 1
    fi

    local reconnect_retry_limit=3
    local saved_networks
    saved_networks=$(wpa_cli -i "$WIFI_IFACE" list_networks 2>/dev/null || true)

    local any_network
    any_network=$(printf '%s\n' "$saved_networks" | awk 'NR>1 && $1 ~ /^[0-9]+$/ {print $1; exit}')
    if [[ -z "$any_network" ]]; then
        echo "ERROR: NO_SAVED_NETWORK"
        return 1
    fi

    # Try last disconnected network first (manual on only)
    local last_ssid=""
    if [[ "$invocation_context" == "manual" && -f "$LAST_WIFI_FILE" ]]; then
        last_ssid=$(<"$LAST_WIFI_FILE")
    fi

    if [[ -n "$last_ssid" ]]; then
        local ssid_escaped
        ssid_escaped=$(escape_non_ascii "$last_ssid")
        local network_id
        network_id=$(printf '%s\n' "$saved_networks" | grep -F "$ssid_escaped" | awk '{print $1}' | head -n1)
        if [[ -n "$network_id" ]]; then
            debug_print "[wifi] turning on, reconnecting to last network: $last_ssid"
            wpa_cli -i "$WIFI_IFACE" set_network "$network_id" mesh_fwding 0 >/dev/null
            wpa_cli -i "$WIFI_IFACE" enable_network "$network_id" >/dev/null
            wpa_cli -i "$WIFI_IFACE" select_network "$network_id" >/dev/null
            wpa_cli -i "$WIFI_IFACE" save_config >/dev/null

            local retry=0
            while [ $retry -lt $reconnect_retry_limit ]; do
                if wpa_cli -i "$WIFI_IFACE" status | grep -qF "wpa_state=COMPLETED"; then
                    debug_print "[wifi] requesting IP address..."
                    if ! configure_sta_ip "$WIFI_IFACE"; then
                        echo "ERROR: DHCP_FAILED"
                        return 1
                    fi
                    if [[ "$persist_boot_marker" == "true" ]]; then
                        write_wifi_boot_config "$last_ssid"
                    fi
                    echo "OK: Connected to $last_ssid"
                    return 0
                fi
                sleep 1
                ((retry++))
            done
            debug_print "[wifi] reconnect to last network failed, trying other saved networks"
        fi
    fi

    # Try any saved network
    if [[ -n "$any_network" ]]; then
        debug_print "[wifi] enabling all saved networks"
        wpa_cli -i "$WIFI_IFACE" enable_network all >/dev/null

        local retry=0
        while [ $retry -lt $reconnect_retry_limit ]; do
            if wpa_cli -i "$WIFI_IFACE" status | grep -qF "wpa_state=COMPLETED"; then
                local connected_ssid
                connected_ssid=$(wpa_cli -i "$WIFI_IFACE" status | grep "^ssid=" | cut -d= -f2)
                debug_print "[wifi] requesting IP address..."
                if ! configure_sta_ip "$WIFI_IFACE"; then
                    echo "ERROR: DHCP_FAILED"
                    return 1
                fi
                if [[ "$persist_boot_marker" == "true" ]]; then
                    write_wifi_boot_config "$connected_ssid"
                fi
                echo "OK: Connected to ${connected_ssid}"
                return 0
            fi
            sleep 1
            ((retry++))
        done
    fi

    echo "ERROR: NO_SAVED_NETWORK"
    return 1
}

turn_off_wifi() {
    if disconnect_wifi; then
        echo "OK: Disconnected"
        return 0
    else
        echo "ERROR: DISCONNECT_FAILED"
        return 1
    fi
}

remove_saved_wifi() {
    local ssid="$1"
    if [[ -z "$ssid" ]]; then
        echo "ERROR: INVALID_ARGS"
        return 1
    fi

    if remove_wifi "$ssid"; then
        echo "OK: Removed ${ssid}"
        return 0
    else
        echo "ERROR: NOT_FOUND"
        return 1
    fi
}

ARGS=()
for arg in "$@"; do
    if [[ "$arg" != "-d" && "$arg" != "--debug" ]]; then
        ARGS+=("$arg")
    fi
done
set -- "${ARGS[@]}"

case "$1" in
"start")
    start_wifi_system
    ;;
"stop")
    stop_wifi_system
    ;;
"restart")
    stop_wifi_system
    start_wifi_system
    ;;
"on")
    turn_on_wifi "manual"
    ;;
"off" | "connect_stop")
    turn_off_wifi
    ;;
"connect" | "connect_start")
    validate_arguments 1 $#
    if [[ -z "$2" ]]; then
        echo "ERROR: INVALID_ARGS"
        exit 1
    fi
    stop_wifi_ap
    disconnect_wifi
    if connect_wifi "$2" "${3:-}"; then
        echo "OK: Connected to $2"
    else
        echo "ERROR: CONNECT_FAILED"
        exit 1
    fi
    ;;
"status")
    get_connection_status
    ;;
"signal")
    get_signal_strength
    ;;
"scan" | "try_scan")
    try_scan
    ;;
"saved")
    get_saved_networks
    ;;
"remove")
    validate_arguments 1 $#
    if [[ -z "$2" ]]; then
        echo "ERROR: INVALID_ARGS"
        exit 1
    fi
    remove_saved_wifi "$2"
    ;;
"clear")
    clear_saved_networks
    ;;
"ap")
    validate_arguments 2 $#
    if start_wifi_ap "$2" "$3"; then
        echo "OK: AP started $2"
    else
        echo "ERROR: AP_FAILED"
        exit 1
    fi
    ;;
"ap_off")
    stop_wifi_ap
    echo "OK: AP stopped"
    ;;
"enterprise")
    validate_arguments 3 $#
    if [[ -z "$2" || -z "$3" || -z "$4" ]]; then
        echo "ERROR: INVALID_ARGS"
        exit 1
    fi
    stop_wifi_ap
    disconnect_wifi
    if connect_enterprise_wifi "$2" "$3" "$4" "${@:5}"; then
        echo "OK: Connected to $2"
    else
        echo "ERROR: CONNECT_FAILED"
        exit 1
    fi
    ;;
"device_count")
    device_check || echo "ERROR: NO_INTERFACE"
    ;;
"ap_ip")
    get_ap_ip
    ;;
"h"|"-h"|"--help")
    show_help
    exit 0
    ;;
*)
    show_help
    exit 1
    ;;
esac
