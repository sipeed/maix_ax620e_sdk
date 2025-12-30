
1. 功能说明：
此Demo用于快启和休眠唤醒功能的演示
此Demo的执行在/etc/init.d/rcS中配置，随系统启动自动执行。

可以通过下面的配置实现自动重启或者自动睡眠
a. 自动重启
   touch /customer/reboot
   Demo板起来后，执行此命令，在customer分区创建reboot的文件。
   系统检测连续5帧检测不到人，会自动reboot。reboot前会将保存的第一帧图像和最多不超过60帧视频保存到sd卡上: /mnt/qsdemo

b. 自动睡眠唤醒（AOV）
   touch /customer/wakeup
   touch /customer/aov
   Demo板起来后，按顺序执行以上命令，在customer分区创建wakeup和aov的文件
   系统检测不到人会将vin的输出帧率降为1fps，然后睡眠1s后唤醒，重复这个过程，直到检测到人，将帧率提高到15fps。
   整个过程会将视频循环保存在sd卡里：/mnt/qsdemo/目录下，如果不插sd卡，只会在tmp目录下保存30帧左右的视频。

注意：reboot和aov这个两个文件，如果同时存在，只会执行快启的reboot

