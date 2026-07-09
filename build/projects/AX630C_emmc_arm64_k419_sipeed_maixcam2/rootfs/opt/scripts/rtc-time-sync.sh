#!/bin/sh

set -eu

PATH=/usr/sbin:/usr/bin:/sbin:/bin
LOG_TAG=rtc-time-sync

# Reject RTC values older than 2024-01-01 UTC unless overridden.
MIN_RTC_EPOCH="${MIN_RTC_EPOCH:-1704067200}"
NETWORK_TIMEOUT="${NETWORK_TIMEOUT:-1}"
NETWORK_PROBE_TARGETS="${NETWORK_PROBE_TARGETS:-223.5.5.5 8.8.8.8}"
NETWORK_INTERFACE="${NETWORK_INTERFACE:-wlan0}"
CHECK_INTERVAL="${CHECK_INTERVAL:-30}"
OFFLINE_CONFIRM_CHECKS="${OFFLINE_CONFIRM_CHECKS:-2}"
RTC_SYNC_INTERVAL="${RTC_SYNC_INTERVAL:-3600}"
MAX_CLOCK_DRIFT="${MAX_CLOCK_DRIFT:-5}"
ALLOW_BACKWARD_STEP="${ALLOW_BACKWARD_STEP:-0}"
RUNTIME_RECHECK_INTERVAL="${RUNTIME_RECHECK_INTERVAL:-10}"

log()
{
	logger -t "$LOG_TAG" "$*" 2>/dev/null || echo "$LOG_TAG: $*"
}

have_network()
{
	if ! interface_is_up; then
		return 1
	fi

	if command -v ping >/dev/null 2>&1; then
		for target in $NETWORK_PROBE_TARGETS; do
			if ping -c 1 -W "$NETWORK_TIMEOUT" "$target" >/dev/null 2>&1; then
				return 0
			fi
		done
		return 1
	fi

	# Fallback for very small images without ping: treat an active default
	# route as network availability.
	awk '$2 == "00000000" && $4 != "0000" { found = 1 } END { exit !found }' \
		/proc/net/route 2>/dev/null
}

interface_is_up()
{
	if [ -z "$NETWORK_INTERFACE" ]; then
		return 0
	fi

	if [ ! -d "/sys/class/net/$NETWORK_INTERFACE" ]; then
		return 1
	fi

	if [ -r "/sys/class/net/$NETWORK_INTERFACE/operstate" ]; then
		read -r operstate < "/sys/class/net/$NETWORK_INTERFACE/operstate" || operstate=unknown
		case "$operstate" in
			up|unknown)
				return 0
				;;
			*)
				return 1
				;;
		esac
	fi

	if [ -r "/sys/class/net/$NETWORK_INTERFACE/carrier" ]; then
		read -r carrier < "/sys/class/net/$NETWORK_INTERFACE/carrier" || carrier=0
		[ "$carrier" = "1" ]
		return
	fi

	return 0
}

current_epoch()
{
	date -u +%s 2>/dev/null
}

abs_diff()
{
	if [ "$1" -gt "$2" ]; then
		echo $(($1 - $2))
	else
		echo $(($2 - $1))
	fi
}

read_rtc_epoch()
{
	for rtc_since_epoch in /sys/class/rtc/rtc*/since_epoch; do
		if [ -r "$rtc_since_epoch" ]; then
			read -r rtc_epoch < "$rtc_since_epoch" || continue
			case "$rtc_epoch" in
				*[!0-9]*|'')
					continue
					;;
				*)
					echo "$rtc_epoch"
					return 0
					;;
			esac
		fi
	done

	if hwclock --get --utc --date '%s' 2>/dev/null; then
		return 0
	fi

	rtc_text="$(hwclock -r -u 2>/dev/null || hwclock -r 2>/dev/null)" || return 1
	date -u -d "$rtc_text" +%s 2>/dev/null
}

validate_runtime()
{
	if [ ! -e /dev/rtc ] && [ ! -e /dev/rtc0 ] && ! ls /sys/class/rtc/rtc*/since_epoch >/dev/null 2>&1; then
		return 1
	fi

	return 0
}

set_system_clock_from_rtc()
{
	if command -v hwclock >/dev/null 2>&1; then
		if hwclock --hctosys --utc 2>/dev/null || hwclock -s -u 2>/dev/null || hwclock -s; then
			return 0
		fi
	fi

	if date -u -s "@$1" >/dev/null 2>&1; then
		return 0
	fi

	return 1
}

sync_from_rtc()
{
	rtc_epoch="$(read_rtc_epoch 2>/dev/null || true)"

	if [ -n "$rtc_epoch" ]; then
		case "$rtc_epoch" in
			*[!0-9]*)
				log "RTC time is not parseable; skip system clock update"
				return 0
				;;
		esac

		if [ "$rtc_epoch" -lt "$MIN_RTC_EPOCH" ]; then
			log "RTC time is older than minimum epoch $MIN_RTC_EPOCH; skip system clock update"
			return 0
		fi
	else
		log "RTC time could not be validated; skip system clock update"
		return 0
	fi

	sys_epoch="$(current_epoch || true)"
	if [ -n "$sys_epoch" ]; then
		case "$sys_epoch" in
			*[!0-9]*)
				sys_epoch=
				;;
		esac
	fi

	if [ -n "$sys_epoch" ] && [ "$sys_epoch" -ge "$MIN_RTC_EPOCH" ]; then
		diff="$(abs_diff "$rtc_epoch" "$sys_epoch")"
		if [ "$diff" -le "$MAX_CLOCK_DRIFT" ]; then
			log "System clock is within ${MAX_CLOCK_DRIFT}s of RTC; skip update"
			return 0
		fi

		if [ "$rtc_epoch" -lt "$sys_epoch" ] && [ "$ALLOW_BACKWARD_STEP" != "1" ]; then
			log "RTC is behind system clock by ${diff}s; skip backward step"
			return 0
		fi
	fi

	if set_system_clock_from_rtc "$rtc_epoch"; then
		log "System clock synchronized from RTC"
		return 0
	fi

	log "Failed to synchronize system clock from RTC"
	return 1
}

run_once()
{
	if ! validate_runtime; then
		log "RTC device is unavailable; skip RTC synchronization"
		return 0
	fi

	if have_network; then
		log "Network is reachable; leave time synchronization to network time service"
		return 0
	fi

	log "Network is unreachable; trying RTC fallback"
	sync_from_rtc
}

monitor()
{
	last_state=unknown
	offline_count=0
	last_sync_epoch=0
	runtime_ready=unknown

	log "Monitoring network interface ${NETWORK_INTERFACE:-any}; check interval ${CHECK_INTERVAL}s"

	while :; do
		if ! validate_runtime; then
			if [ "$runtime_ready" != "no" ]; then
				log "RTC runtime is not ready; keep service alive and retry"
				runtime_ready=no
			fi
			sleep "$RUNTIME_RECHECK_INTERVAL"
			continue
		fi

		if [ "$runtime_ready" != "yes" ]; then
			log "RTC runtime is ready"
			runtime_ready=yes
		fi

		if have_network; then
			offline_count=0
			if [ "$last_state" != "online" ]; then
				log "Network is reachable; suspend RTC fallback"
				last_state=online
			fi
		else
			offline_count=$((offline_count + 1))
			if [ "$offline_count" -ge "$OFFLINE_CONFIRM_CHECKS" ]; then
				now_epoch="$(current_epoch || echo 0)"
				if [ "$last_state" != "offline" ]; then
					log "Network is unreachable after ${OFFLINE_CONFIRM_CHECKS} checks; enable RTC fallback"
					last_state=offline
				fi

				if [ "$last_sync_epoch" -eq 0 ] || [ $((now_epoch - last_sync_epoch)) -ge "$RTC_SYNC_INTERVAL" ]; then
					if sync_from_rtc; then
						last_sync_epoch="$(current_epoch || echo "$now_epoch")"
					fi
				fi
			fi
		fi

		sleep "$CHECK_INTERVAL"
	done
}

main()
{
	case "${1:---once}" in
		--monitor)
			monitor
			;;
		--once)
			run_once
			;;
		*)
			echo "Usage: $0 [--once|--monitor]" >&2
			return 2
			;;
	esac
}

main "$@"
