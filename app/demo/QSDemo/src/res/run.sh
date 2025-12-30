#!/bin/sh
app=qsdemo_s

# (a) aiisp mode
# 0: TISP
# 1: AIISP default
# 2: Manual AIISP
# 3: Manual TISP
# 4: Auto AIISP
aiisp_mode=1

# (b) vin ivps mode
# 1: gdc online vpp
# 2: itp onle vpp
vinivps_mode=2

# (c) Reboot Interval from main lanch
# unit sencond
# 0: reboot if no body found 5fps
# >0: reboot if timeout n sencods
# need insert sd card
reboot_interval=0

# (d) sdr/hdr switch times
# 0: no switch
# >0: switch N times, interval 60s
sdrhdr_switch_test=0

# (e) if enable jenc
# 1: enable
# 0: disable and not init jenc
jenc_enable=1

# (f) set aov vin cap num when wakeup
# 0: not set, defaut 1fps
# 1~3: cap num
# 4: for test, randam switch between 1~3
vin_cap_num=0

# (g) sc850sl output 2M, only for sc850sl sensor
# 0: disable
# 1: eanble
# if enable, need set env and reboot: ax_env.sh set qs_sc850sl2m 1
sc850sl2m=0

# (i) audio flag
# 0: disable
# 1: enable audio capture
# 2: enable audio play
# 3: enable audio capture and play
audio_enable=0

# (j) rtsp enable
# 0: disable
# 1: enable
rtsp_enable=1

# (k) aov one cycle total time
# unit ms
sleep_time=1000

# (l) vin channel frame mode
# 0: off
# 1: ring
chn_frame_mode=1

# (m) detect algo
# 0: skel
# 1: md
# 2: md->skel
detect_algo=0

# (n) gdc online vpp test
# 0: disable
# 1: enable
gdconlinevpp_test=0

# (o) use tisp in aov (1fps)
# 0: disable
# 1: enable
tisp_in_aov=0

# (p) store audio capture in aov
# 0: disable
# 1: enable
audio_in_aov=0

# (q) change venc rc policy in aov
# 0: change bitrate
# 1: change gop
venc_change_rc_policy=0

# (r) max record file count
max_record_file_count=5

# (s) venc gop in aov
gop_in_aov=10

# (t) ezoom switch info (x,y,w,h,cx,cy)
ezoom_info=422,278,922,518,-78,-4

# (u) slow shutter
# -1: use default setting
# 0 : off
# 1 : on
slow_shutter=-1

# (v) ezoom test
# 0 : off
# 1 : on
ezoom_test=0

# (w) ae manual shutter (us)
# -1 : default
# >0 : manual
ae_manual_shutter=-1

# launch
${app} -a ${aiisp_mode} -b ${vinivps_mode} -c ${reboot_interval} -d ${sdrhdr_switch_test} -e ${jenc_enable} -f ${vin_cap_num} -g ${sc850sl2m} -i ${audio_enable} -j ${rtsp_enable} -k ${sleep_time} -l ${chn_frame_mode} -m ${detect_algo} -n ${gdconlinevpp_test} -o ${tisp_in_aov} -p ${audio_in_aov} -q ${venc_change_rc_policy} -r ${max_record_file_count} -s ${gop_in_aov} -t ${ezoom_info} -u ${slow_shutter} -v ${ezoom_test} -w ${ae_manual_shutter} &

