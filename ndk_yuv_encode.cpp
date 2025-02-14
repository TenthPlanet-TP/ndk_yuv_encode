#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <limits>
#include <string>
#include <thread>
#include <time.h>

#if defined(ANDROID_7)
#include <ndk/NdkMediaCodec.h>
#include <ndk/NdkMediaFormat.h>
#elif defined(ANDROID_10)
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#endif

// #include <android/log.h>
#include <sys/system_properties.h>


// frameworks/av/include/media/stagefright/MediaCodecConstants.h
constexpr int32_t BITRATE_MODE_CBR = 2;
constexpr int32_t BITRATE_MODE_CQ = 0;
constexpr int32_t BITRATE_MODE_VBR = 1;

constexpr int32_t AVCProfileBaseline = 0x01;
constexpr int32_t AVCProfileMain     = 0x02;
constexpr int32_t AVCProfileExtended = 0x04;
constexpr int32_t AVCProfileHigh     = 0x08;
constexpr int32_t AVCProfileHigh10   = 0x10;
constexpr int32_t AVCProfileHigh422  = 0x20;
constexpr int32_t AVCProfileHigh444  = 0x40;
constexpr int32_t AVCProfileConstrainedBaseline = 0x10000;
constexpr int32_t AVCProfileConstrainedHigh     = 0x80000;

constexpr int32_t AVCLevel1       = 0x01;
constexpr int32_t AVCLevel1b      = 0x02;
constexpr int32_t AVCLevel11      = 0x04;
constexpr int32_t AVCLevel12      = 0x08;
constexpr int32_t AVCLevel13      = 0x10;
constexpr int32_t AVCLevel2       = 0x20;
constexpr int32_t AVCLevel21      = 0x40;
constexpr int32_t AVCLevel22      = 0x80;
constexpr int32_t AVCLevel3       = 0x100;
constexpr int32_t AVCLevel31      = 0x200;
constexpr int32_t AVCLevel32      = 0x400;
constexpr int32_t AVCLevel4       = 0x800;
constexpr int32_t AVCLevel41      = 0x1000;
constexpr int32_t AVCLevel42      = 0x2000;
constexpr int32_t AVCLevel5       = 0x4000;
constexpr int32_t AVCLevel51      = 0x8000;
constexpr int32_t AVCLevel52      = 0x10000;
constexpr int32_t AVCLevel6       = 0x20000;
constexpr int32_t AVCLevel61      = 0x40000;
constexpr int32_t AVCLevel62      = 0x80000;

constexpr int32_t HEVCProfileMain        = 0x01;
constexpr int32_t HEVCProfileMain10      = 0x02;
constexpr int32_t HEVCProfileMainStill   = 0x04;
constexpr int32_t HEVCProfileMain10HDR10 = 0x1000;
constexpr int32_t HEVCProfileMain10HDR10Plus = 0x2000;

constexpr int32_t HEVCMainTierLevel1  = 0x1;
constexpr int32_t HEVCHighTierLevel1  = 0x2;
constexpr int32_t HEVCMainTierLevel2  = 0x4;
constexpr int32_t HEVCHighTierLevel2  = 0x8;
constexpr int32_t HEVCMainTierLevel21 = 0x10;
constexpr int32_t HEVCHighTierLevel21 = 0x20;
constexpr int32_t HEVCMainTierLevel3  = 0x40;
constexpr int32_t HEVCHighTierLevel3  = 0x80;
constexpr int32_t HEVCMainTierLevel31 = 0x100;
constexpr int32_t HEVCHighTierLevel31 = 0x200;
constexpr int32_t HEVCMainTierLevel4  = 0x400;
constexpr int32_t HEVCHighTierLevel4  = 0x800;
constexpr int32_t HEVCMainTierLevel41 = 0x1000;
constexpr int32_t HEVCHighTierLevel41 = 0x2000;
constexpr int32_t HEVCMainTierLevel5  = 0x4000;
constexpr int32_t HEVCHighTierLevel5  = 0x8000;
constexpr int32_t HEVCMainTierLevel51 = 0x10000;
constexpr int32_t HEVCHighTierLevel51 = 0x20000;
constexpr int32_t HEVCMainTierLevel52 = 0x40000;
constexpr int32_t HEVCHighTierLevel52 = 0x80000;
constexpr int32_t HEVCMainTierLevel6  = 0x100000;
constexpr int32_t HEVCHighTierLevel6  = 0x200000;
constexpr int32_t HEVCMainTierLevel61 = 0x400000;
constexpr int32_t HEVCHighTierLevel61 = 0x800000;
constexpr int32_t HEVCMainTierLevel62 = 0x1000000;
constexpr int32_t HEVCHighTierLevel62 = 0x2000000;

/// frameworks/native/include/media/openmax/OMX_IVCommon.h
typedef enum OMX_COLOR_FORMATTYPE {
  OMX_COLOR_FormatUnused,
  OMX_COLOR_FormatYUV420Planar = 19,
  OMX_COLOR_FormatYUV420SemiPlanar = 21,
  OMX_COLOR_FormatMax = 0x7FFFFFFF
} OMX_COLOR_FORMATTYPE;


// system/core/libutils/include/utils/Timers.h
typedef int64_t nsecs_t;       // nano-seconds

#if defined(__ANDROID__)
nsecs_t systemTime(int clock)
{
    static const clockid_t clocks[] = {
            CLOCK_REALTIME,
            CLOCK_MONOTONIC,
            CLOCK_PROCESS_CPUTIME_ID,
            CLOCK_THREAD_CPUTIME_ID,
            CLOCK_BOOTTIME
    };
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(clocks[clock], &t);
    return nsecs_t(t.tv_sec)*1000000000LL + t.tv_nsec;
}
#else
nsecs_t systemTime(int /*clock*/)
{
    // Clock support varies widely across hosts. Mac OS doesn't support
    // posix clocks, older glibcs don't support CLOCK_BOOTTIME and Windows
    // is windows.
    struct timeval t;
    t.tv_sec = t.tv_usec = 0;
    gettimeofday(&t, nullptr);
    return nsecs_t(t.tv_sec)*1000000000LL + nsecs_t(t.tv_usec)*1000LL;
}
#endif

enum {
  SYSTEM_TIME_REALTIME = 0,  // system-wide realtime clock
  SYSTEM_TIME_MONOTONIC = 1, // monotonic time since unspecified starting point
  SYSTEM_TIME_PROCESS = 2,   // high-resolution per-process clock
  SYSTEM_TIME_THREAD = 3,    // high-resolution per-thread clock
  SYSTEM_TIME_BOOTTIME = 4   // same as SYSTEM_TIME_MONOTONIC, but including CPU suspend time
};


// WebRTC queues input frames quickly in the beginning on the call. Wait for input buffers with a
// long timeout (500 ms) to prevent this from causing the codec to return an error.
#define DEQUEUE_INPUT_TIMEOUT_US            500000

// Dequeuing an output buffer will block until a buffer is available (up to 100 milliseconds).
// If this timeout is exceeded, the output thread will unblock and check if the decoder is still
// running.  If it is, it will block on dequeue again.  Otherwise, it will stop and release the
// MediaCodec.
#define DEQUEUE_OUTPUT_BUFFER_TIMEOUT_US    100000



int getVersionSdk() {
  // 1. 获取 SDK 版本号 , 存储于 C 字符串 sdk_verison_str 中
  char sdk[128] = "0";

  // 获取版本号方法
  __system_property_get("ro.build.version.sdk", sdk);

  //将版本号转为 int 值
  int sdk_verison = atoi(sdk);
  return sdk_verison;
}

int main(int argc, const char* argv[]) {
  if (argc != 10) {
    printf("Usage: %s <width> <height> <codec> <bitrate> <framerate> <min_qp> <max_qp> <input_path> <output_path>\n", argv[0]);
    return 0;
  }

  int width = std::atoi(argv[1]);
  int height = std::atoi(argv[2]);
  std::string codec = std::string("video/") + argv[3];  // hevc/avc
  int bitrate = std::atoi(argv[4]);
  int framerate = std::atoi(argv[5]);
  int min_qp = std::atoi(argv[6]);
  int max_qp = std::atoi(argv[7]);
  const char* input_path = argv[8];
  const char* output_path = argv[9];

  if (min_qp > 51 || max_qp > 51) {
    printf("error, qp: [0, 51], min_qp: %d, max_qp: %d\n", min_qp, max_qp);
    return -1;
  }

  int sdk_verison = getVersionSdk();
  printf("sdk_verison: %d\n", sdk_verison);

  AMediaFormat* format = AMediaFormat_new();
  AMediaFormat_setString(format, "mime", codec.c_str());
  AMediaFormat_setInt32(format, "width", width);
  AMediaFormat_setInt32(format, "height", height);
  AMediaFormat_setInt32(format, "color-format", OMX_COLOR_FormatYUV420Planar);
  AMediaFormat_setInt32(format, "bitrate", bitrate);
  AMediaFormat_setInt32(format, "max-bitrate", bitrate);
  AMediaFormat_setInt32(format, "bitrate-mode", BITRATE_MODE_CBR);
  AMediaFormat_setInt32(format, "i-frame-interval", 360000);
  AMediaFormat_setInt32(format, "max-bframes", 0);
  AMediaFormat_setFloat(format, "frame-rate", framerate);

  if (min_qp >= 0 && max_qp >= 0) {
    // Qcom
    AMediaFormat_setInt32(format, "vendor.qti-ext-enc-initial-qp.qp-i-enable", 1);
    AMediaFormat_setInt32(format, "vendor.qti-ext-enc-initial-qp.qp-i", min_qp);
    AMediaFormat_setInt32(format, "video-qp-i-min", min_qp);
    AMediaFormat_setInt32(format, "video-qp-i-max", max_qp);

    AMediaFormat_setInt32(format, "vendor.qti-ext-enc-initial-qp.qp-p-enable", 1);
    AMediaFormat_setInt32(format, "vendor.qti-ext-enc-initial-qp.qp-p", min_qp);
    AMediaFormat_setInt32(format, "video-qp-p-min", min_qp);
    AMediaFormat_setInt32(format, "video-qp-p-max", max_qp);
  }

  if (codec == "video/avc") {
    AMediaFormat_setInt32(format, "profile", AVCProfileHigh);
    AMediaFormat_setInt32(format, "level", AVCLevel4);
  } else if (codec == "video/hevc") {
    AMediaFormat_setInt32(format, "profile", HEVCProfileMain);
    AMediaFormat_setInt32(format, "level", HEVCMainTierLevel4);
  } else {
    assert(0);
  }

  // Qcom
  // AMediaFormat_setInt32(format, "vendor.qti-ext-enc-low-latency.enable", 1);

#if 0
  if (sdk_verison >= 13) {
    // 启用编码统计功能
    AMediaFormat_setInt32(format, "video-encoding-statistics-level", 1); // LEVEL_1
  }
#endif

  printf("format: %s\n", AMediaFormat_toString(format));

  AMediaCodec* encoder = AMediaCodec_createEncoderByType(codec.c_str());
  if (encoder == nullptr) {
    printf("AMediaCodec_createEncoderByType failed\n");
    AMediaFormat_delete(format);
    return -1;
  }

  media_status_t status = AMediaCodec_configure(encoder, format, nullptr, nullptr, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
  if (status != AMEDIA_OK) {
    printf("AMediaCodec_configure failed\n");
    AMediaCodec_delete(encoder);
    AMediaFormat_delete(format);
    return -1;
  }

  status = AMediaCodec_start(encoder);
  if (status != AMEDIA_OK) {
    printf("AMediaCodec_start failed\n");
    AMediaCodec_delete(encoder);
    AMediaFormat_delete(format);
    return -1;
  }

  if (format != nullptr) {
	  AMediaFormat_delete(format);
  }

  std::ifstream input_file(input_path, std::ios::binary);
  std::ofstream output_file(output_path, std::ios::binary);

  int frame_number = 0;
  nsecs_t input_pts = systemTime(SYSTEM_TIME_MONOTONIC) / 1000;
  bool ended = false;
  int64_t timeoutUs = 0;	// 0 为不等待，-1 为一直等待，其余为时间单位
  while (true) {
    ssize_t buf_index = AMediaCodec_dequeueInputBuffer(encoder, timeoutUs);
    if (buf_index >= 0) {
      size_t buf_size;
      uint8_t* buf = AMediaCodec_getInputBuffer(encoder, buf_index, &buf_size);
      if (input_file.eof()) {
        ended = true;
        AMediaCodec_queueInputBuffer(encoder, buf_index, 0, 0, input_pts, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
      } else {
        input_file.read(reinterpret_cast<char*>(buf), width * height * 3 / 2);
        AMediaCodec_queueInputBuffer(encoder, buf_index, 0, width * height * 3 / 2, input_pts, 0);
        input_pts += 1000000 / framerate;
      }
    }

    AMediaCodecBufferInfo info;
    ssize_t out_buf_index = AMediaCodec_dequeueOutputBuffer(encoder, &info, DEQUEUE_OUTPUT_BUFFER_TIMEOUT_US);
    if (out_buf_index >= 0) {
      size_t out_buf_size;
      uint8_t* out_buf = AMediaCodec_getOutputBuffer(encoder, out_buf_index, &out_buf_size);
      output_file.write(reinterpret_cast<char*>(out_buf), info.size);
      output_file.flush();
      AMediaCodec_releaseOutputBuffer(encoder, out_buf_index, false);
      if ((info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0) {
        break;
      }
      ++frame_number;
    } else if (out_buf_index == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
      AMediaFormat* new_format = AMediaCodec_getOutputFormat(encoder);
#if 1
	    printf("\nnew format: %s\n", AMediaFormat_toString(new_format));
#else
      if (sdk_verison >= 13) {
        // 提取 QP 平均值
        int32_t qpAverage;
        if (AMediaFormat_getInt32(new_format, "video-qp-average", &qpAverage)) {
            // if (qpAverage == INT_MAX) {
            //   printf("Frame %lld: All blocks in skip mode\n", (long long)info.presentationTimeUs);
            // } else {
            //   printf("Frame %lld: Average QP = %d\n", (long long)info.presentationTimeUs, qpAverage);
            // }
        }
        
        // 提取图片类型
        int32_t pictureType;
        if (AMediaFormat_getInt32(new_format, "picture-type", &pictureType)) {

        }

        printf("EncStats: QP: %lld, type: %d\n", qpAverage, pictureType);
      }
#endif
      AMediaFormat_delete(new_format);
    } else if (out_buf_index == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
      if (ended) {
        break;
      }
    }
  }

  printf("\nencode end, Frame number %d\n", frame_number);

  input_file.close();
  output_file.close();

  if (encoder != nullptr) {
  	AMediaCodec_stop(encoder);
  	AMediaCodec_delete(encoder);
  }

  return 0;
}
