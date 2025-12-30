#ifndef _AX_DETECT_H_
#define _AX_DETECT_H_

#define ENGINE_THREAD_STACK_SIZE    4096
#define ENGINE_THREAD_PRIORITY      9
#define ENGINE_THREAD_TICK_SLICE    5

#define ALG_DET_CSS_NUM             1
#define ALG_DET_NMS_THR             0.6f
#define ALG_DET_MIN_CONF            0.5f
#define ALG_DET_FRAME_WIDTH         960
#define ALG_DET_FRAME_HEIGHT        544

#define COMMON_SYS_GLB_BASE     0x2340000
#define CHIP_SW_RST_SET         0xAC
#define DETECT_MAX_COUNT        (4)

#define FW_ENV_RAW_DET_COUNT    "raw_det_count"

typedef enum {
    AX_YUV_DETECT = 0,
    AX_RAW_DETECT,
    AX_MD_DETECT,
} AX_DETECT_TYPE_E;;

int ax_detect_type(void);

#endif //_AX_DETECT_H_

