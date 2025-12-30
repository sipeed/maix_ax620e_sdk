#OPALDemo

## 如何执行？

1. cd /opt/bin/OPALDemo
2. 执行./run.sh [参数说明](#启动参数)

> 添加-d用于gdb调试，内部使用了SIGUSR2信号作为定时器，因此gdb调试时输入**handle SIGUSR2 nostop noprint**忽略SIGUSR2信号。

## 如何编译？

1. cd app/demo/OPALDemo
2. make p=xxx clean
3. make p=xxx
4. make p=xxx install

> p=xxx 指定编译项目名，示例：make p=AX630C_emmc_arm64_k419

## 如何预览？

1. RTSP流预览：可通过第三方工具，比如VLC，输入RTSP流地址（参照终端打印输出：“Play the stream using url: <<<<< rtsp://IP:8554/axstream0 >>>>>”）进行预览。
2. WEB预览：可通过Chrome打开网页（参照终端打印输出：“Preview the video using URL: <<<<< http://IP:8080 >>>>>”）进行预览。

# <a href="#启动参数">启动参数</a>

l: log level, indicates the log level.
CRITICAL = 1, ERROR = 2 (DEFAULT), WARN = 3, NOTICE = 4, INFO = 5, DEBUG = 6, DATA = 7
t: log target, indicates the log output targets.
SYSLOG = 1, APPLOG = 2, STDOUT = 4 (DEFAULT) (Calculate the sum if multiple targets is required)
d: start with gdb for debugging, value **NOT REQUIRED**
u: testsuite type.
0: Dual default
1: Single default

# <a href="#配置参数">配置参数</a>


| # | 参数                | 参数范围      | 说明                           |
| - | ------------------- | ------------- | ------------------------------ |
| 1 | RTSPMaxFrmSize      | [0 - 8000000] | 单位：B                        |
| 2 | WebVencFrmSizeRatio |               | Web缓存帧相对YUV的size比例     |
| 3 | WebJencFrmSizeRatio |               | Web缓存帧相对YUV的size比例     |
| 4 | PrintFPS            |               | 0：不打印 1:打印               |
| 5 | EnableOSD           |               | 0：关闭OSD 1:开启OSD           |
| 6 | EnableMp4Record     |               | 0：关闭板端录像 1:开启板端录像 |
| 7 | MP4RecordSavedPath  |               | 板端录像保存路径               |
