#ifndef _MP4_API_H_
#define _MP4_API_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// mp4 data type
typedef enum {
    MP4_DATA_VIDEO,
    MP4_DATA_AUDIO,
    MP4_DATA_BUTT
} mp4_data_e;

// mp4 status
typedef enum {
    MP4_STATUS_NONE,
    MP4_STATUS_START,
    MP4_STATUS_COMPLETE,
    MP4_STATUS_DELETED,
    MP4_STATUS_FAILURE,
    MP4_STATUS_DISK_FULL,
    MP4_STATUS_BUTT
} mp4_status_e;

// mp4 object type
typedef enum {
    // none
    MP4_OBJECT_NONE,

    // video
    MP4_OBJECT_AVC,
    MP4_OBJECT_HEVC,
    MP4_OBJECT_VVC, //not support now

    // audio
    MP4_OBJECT_ALAW,
    MP4_OBJECT_ULAW,
    MP4_OBJECT_G726, // not support now
    MP4_OBJECT_OPUS, // not support now
    MP4_OBJECT_AAC,

    MP4_OBJECT_BUTT
} mp4_object_e;

// mp4 handle
typedef void* MP4_HANDLE;

// mp4 status report callback
typedef void (*mp4_status_report_callback)(MP4_HANDLE handle, const char *file_name, mp4_status_e status, void *user_data);

// video attribute
typedef struct _mp4_video_t {
    bool enable;
    mp4_object_e object;
    int framerate;
    int bitrate;
    int width;
    int height;
    int depth; // default: 5
    int max_frm_size;  // default: width * height * 3 / 2 / 8
} mp4_video_t;

// audio attribute
typedef struct _mp4_audio_t {
    bool enable;
    mp4_object_e object;
    int samplerate;
    int bitrate;
    int chncnt;
    int aot; // audio object type
    int depth;  // default: 5
    int max_frm_size; // default: 2048
} mp4_audio_t;

// mp4 information
typedef struct _mp4_info_t {
    bool loop;
    unsigned int max_file_size; // MiB (default: 256 MiB)
    unsigned int max_file_num; // (default: 10)
    char *dest_path;
    char *file_name_prefix; // file name: file_name_prefix + "yyyy-mm-dd-hh-mm-ss.mp4"
    void *user_data;
    mp4_video_t video;
    mp4_audio_t audio;
    mp4_status_report_callback sr_callback;
} mp4_info_t;

//////////////////////////////////////////////////////////////////////////////////////
/// @brief create mp4 handle
///
/// @param [IN] mp4_info:    mp4 information
///
/// @return handle if success, null if failure
//////////////////////////////////////////////////////////////////////////////////////
MP4_HANDLE mp4_create(const mp4_info_t* mp4_info);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief destroy mp4 handle
///
/// @param [IN] handle:    mp4 handle
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
int mp4_destroy(MP4_HANDLE handle);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief send mp4 frame
///
/// @param [IN] handle:    mp4 handle
///        [IN] type:      data type
///        [IN] data:      media data
///        [IN] bytes:     data length
///        [IN] iframe:    iframe(for video)
///        [IN] pts:       pts
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
int mp4_send(MP4_HANDLE handle, mp4_data_e type, const void* data, int bytes, unsigned long long int pts, bool iframe);

#ifdef __cplusplus
}
#endif
#endif
